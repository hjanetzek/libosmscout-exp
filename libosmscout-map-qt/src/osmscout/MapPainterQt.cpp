/*
  This source is part of the libosmscout-map library
  Copyright (C) 2009  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscout/MapPainterQt.h>

#include <iostream>
#include <limits>

#include <QPainterPath>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/Geometry.h>

namespace osmscout {

  MapPainterQt::MapPainterQt(const StyleConfigRef& styleConfig)
  : MapPainter(styleConfig,
               new CoordBufferImpl<Vertex2D>()),
    coordBuffer((CoordBufferImpl<Vertex2D>*)transBuffer.buffer),
    painter(NULL)
  {
    sin.resize(360*10);

    for (size_t i=0; i<sin.size(); i++) {
      sin[i]=std::sin(M_PI/180*i/(sin.size()/360));
    }
  }

  MapPainterQt::~MapPainterQt()
  {
    // no code
    // TODO: Clean up fonts
  }

  QFont MapPainterQt::GetFont(const MapParameter& parameter,
                              double fontSize)
  {
    std::map<size_t,QFont>::const_iterator f;

    fontSize=fontSize*ConvertWidthToPixel(parameter,parameter.GetFontSize());

    f=fonts.find(fontSize);

    if (f!=fonts.end()) {
      return f->second;
    }

    QFont font(parameter.GetFontName().c_str(),QFont::Normal,false);

    font.setPixelSize(fontSize);
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setStyleStrategy(QFont::PreferMatch);

    return fonts.insert(std::pair<size_t,QFont>(fontSize,font)).first->second;
  }

  bool MapPainterQt::HasIcon(const StyleConfig& /*styleConfig*/,
                             const MapParameter& parameter,
                             IconStyle& style)
  {
    if (style.GetIconId()==0) {
      return false;
    }

    size_t idx=style.GetIconId()-1;

    if (idx<images.size() &&
        !images[idx].isNull()) {
      return true;
    }

    for (std::list<std::string>::const_iterator path=parameter.GetIconPaths().begin();
         path!=parameter.GetIconPaths().end();
         ++path) {

      std::string filename=*path+style.GetIconName()+".png";

      QImage image;

      if (image.load(filename.c_str())) {
        if (idx>=images.size()) {
          images.resize(idx+1);
        }

        images[idx]=image;

        std::cout << "Loaded image '" << filename << "'" << std::endl;

        return true;
      }
    }

    std::cerr << "ERROR while loading image '" << style.GetIconName() << "'" << std::endl;
    style.SetIconId(0);

    return false;
  }

  bool MapPainterQt::HasPattern(const MapParameter& parameter,
                                const FillStyle& style)
  {
    assert(style.HasPattern());

    // Was not able to load pattern
    if (style.GetPatternId()==0) {
      return false;
    }

    size_t idx=style.GetPatternId()-1;

    if (idx<patterns.size() &&
        !patternImages[idx].isNull()) {
      return true;
    }

    for (std::list<std::string>::const_iterator path=parameter.GetPatternPaths().begin();
         path!=parameter.GetPatternPaths().end();
         ++path) {
      std::string filename=*path+style.GetPatternName()+".png";

      QImage image;

      if (image.load(filename.c_str())) {
        if (idx>=patternImages.size()) {
          patternImages.resize(idx+1);
        }

        patternImages[idx]=image;

        if (idx>=patterns.size()) {
          patterns.resize(idx+1);
        }

        patterns[idx].setTextureImage(image);

        std::cout << "Loaded image '" << filename << "'" << std::endl;

        return true;
      }
    }

    std::cerr << "ERROR while loading image '" << style.GetPatternName() << "'" << std::endl;
    style.SetPatternId(0);

    return false;
  }

  void MapPainterQt::GetTextDimension(const MapParameter& parameter,
                                      double fontSize,
                                      const std::string& text,
                                      double& xOff,
                                      double& yOff,
                                      double& width,
                                      double& height)
  {
    QFont        font(GetFont(parameter,fontSize));
    QFontMetrics metrics=QFontMetrics(font);
    QString      string=QString::fromUtf8(text.c_str());
    QRect        extents=metrics.boundingRect(string);

    xOff=extents.x();
    yOff=extents.y();
    width=extents.width();
    height=extents.height();
  }

  void MapPainterQt::DrawLabel(const Projection& /*projection*/,
                               const MapParameter& parameter,
                               const LabelData& label)
  {
    if (dynamic_cast<const TextStyle*>(label.style.Get())!=NULL) {
      const TextStyle* style=dynamic_cast<const TextStyle*>(label.style.Get());
      double           r=style->GetTextColor().GetR();
      double           g=style->GetTextColor().GetG();
      double           b=style->GetTextColor().GetB();

      QFont        font(GetFont(parameter,label.fontSize));
      QString      string=QString::fromUtf8(label.text.c_str());
      QFontMetrics metrics=QFontMetrics(font);

      if (style->GetStyle()==TextStyle::normal) {
        painter->setPen(QColor::fromRgbF(r,g,b,label.alpha));
        painter->setBrush(Qt::NoBrush);
        painter->setFont(font);
        painter->drawText(QPointF(label.x,
                                  label.y+metrics.ascent()),
                          string);
      }
      else if (style->GetStyle()==TextStyle::emphasize) {
        QPainterPath path;
        QPen         pen;

        pen.setColor(QColor::fromRgbF(1.0,1.0,1.0,label.alpha));
        pen.setWidthF(2.0);

        painter->setPen(pen);

        path.addText(QPointF(label.x,
                             label.y+metrics.ascent()),
                             font,
                     string);

        painter->drawPath(path);
        painter->fillPath(path,QBrush(QColor::fromRgbF(r,g,b,label.alpha)));
      }
    }
    else if (dynamic_cast<const ShieldStyle*>(label.style.Get())!=NULL) {
      const ShieldStyle* style=dynamic_cast<const ShieldStyle*>(label.style.Get());
      QFont              font(GetFont(parameter,label.fontSize));
      QFontMetrics       metrics=QFontMetrics(font);
      QString            string=QString::fromUtf8(label.text.c_str());

      painter->fillRect(QRectF(label.bx1,
                               label.by1,
                               label.bx2-label.bx1+1,
                               label.by2-label.by1+1),
                        QBrush(QColor::fromRgbF(style->GetBgColor().GetR(),
                                                style->GetBgColor().GetG(),
                                                style->GetBgColor().GetB(),
                                                1)));

      painter->setPen(QColor::fromRgbF(style->GetBorderColor().GetR(),
                                       style->GetBorderColor().GetG(),
                                       style->GetBorderColor().GetB(),
                                       style->GetBorderColor().GetA()));
      painter->setBrush(Qt::NoBrush);

      painter->drawRect(QRectF(label.bx1+2,
                               label.by1+2,
                               label.bx2-label.bx1+1-4,
                               label.by2-label.by1+1-4));

      painter->setPen(QColor::fromRgbF(style->GetTextColor().GetR(),
                                       style->GetTextColor().GetG(),
                                       style->GetTextColor().GetB(),
                                       style->GetTextColor().GetA()));
      painter->setBrush(Qt::NoBrush);
      painter->setFont(font);
      painter->drawText(QPointF(label.x,
                                label.y+metrics.ascent()),
                                string);
    }
  }

  void MapPainterQt::DrawContourLabel(const Projection& /*projection*/,
                                      const MapParameter& parameter,
                                      const PathTextStyle& style,
                                      const std::string& text,
                                      size_t transStart, size_t transEnd)
  {
    double fontSize=style.GetSize();
    double r=style.GetTextColor().GetR();
    double g=style.GetTextColor().GetG();
    double b=style.GetTextColor().GetB();
    double a=style.GetTextColor().GetA();

    QPen          pen;
    QFont         font(GetFont(parameter,fontSize));
    QFontMetricsF metrics=QFontMetricsF(font,painter->device());
    QString       string=QString::fromUtf8(text.c_str());
    double        stringLength=metrics.width(string);

    pen.setColor(QColor::fromRgbF(r,g,b,a));
    painter->setPen(pen);
    painter->setFont(font);

    QPainterPath p;

    if (coordBuffer->buffer[transStart].GetX()<coordBuffer->buffer[transEnd].GetX()) {
      for (size_t j=transStart; j<=transEnd; j++) {
        if (j==transStart) {
          p.moveTo(coordBuffer->buffer[j].GetX(),
                   coordBuffer->buffer[j].GetY());
        }
        else {
          p.lineTo(coordBuffer->buffer[j].GetX(),
                   coordBuffer->buffer[j].GetY());
        }
      }
    }
    else {
      for (size_t j=0; j<=transEnd-transStart; j++) {
        size_t idx=transEnd-j;

        if (j==0) {
          p.moveTo(coordBuffer->buffer[idx].GetX(),
                   coordBuffer->buffer[idx].GetY());
        }
        else {
          p.lineTo(coordBuffer->buffer[idx].GetX(),
                   coordBuffer->buffer[idx].GetY());
        }
      }
    }

    if (p.length()<stringLength) {
      // Text is longer than path to draw on
      return;
    }

    qreal offset=(p.length()-stringLength)/2;

    QTransform tran;

    for (int i=0; i<string.size(); i++) {
      QPointF point=p.pointAtPercent(p.percentAtLength(offset));
      qreal angle=p.angleAtPercent(p.percentAtLength(offset));

      // rotation matrix components

      qreal sina=sin[lround((360-angle)*10)%sin.size()];
      qreal cosa=sin[lround((360-angle+90)*10)%sin.size()];

      // Rotation
      qreal newX=(cosa*point.x())-(sina*point.y());
      qreal newY=(cosa*point.y())+(sina*point.x());

      // Aditional offseting
      qreal deltaPenX=cosa*pen.width();
      qreal deltaPenY=sina*pen.width();

      // Getting the delta distance for the translation part of the transformation
      qreal deltaX=newX-point.x();
      qreal deltaY=newY-point.y();

      // Applying rotation and translation.
      tran.setMatrix(cosa,sina,0.0,
                     -sina,cosa,0.0,
                     -deltaX+deltaPenX,-deltaY-deltaPenY,1.0);

      painter->setTransform(tran);

      painter->drawText(point,QString(string[i]));

      offset+=metrics.width(string[i]);
    }

    painter->resetTransform();
  }


  void MapPainterQt::followPathInit(FollowPathHandle &hnd, Vertex2D &origin, size_t transStart, size_t transEnd,
                                     bool isClosed, bool keepOrientation) {
      hnd.i = 0;
      hnd.nVertex = labs(transEnd - transStart);
      bool isReallyClosed = (coordBuffer->buffer[transStart] == coordBuffer->buffer[transEnd]);
      if(isClosed && !isReallyClosed){
          hnd.nVertex++;
          hnd.closeWay = true;
      } else {
          hnd.closeWay = false;
      }
      if(keepOrientation || coordBuffer->buffer[transStart].GetX()<coordBuffer->buffer[transEnd].GetX()){
          hnd.transStart = transStart;
          hnd.transEnd = transEnd;
      } else {
          hnd.transStart = transEnd;
          hnd.transEnd = transStart;
      }
      hnd.direction = (hnd.transStart < hnd.transEnd) ? 1 : -1;
      origin.Set(coordBuffer->buffer[hnd.transStart].GetX(), coordBuffer->buffer[hnd.transStart].GetY());
  }

  bool MapPainterQt::followPath(FollowPathHandle &hnd, double l, Vertex2D &origin) {

      double x = origin.GetX();
      double y = origin.GetY();
      double x2,y2;
      double deltaX, deltaY, len, fracToGo;
      while(hnd.i < hnd.nVertex) {
          if(hnd.closeWay && hnd.nVertex - hnd.i == 1){
              x2 = coordBuffer->buffer[hnd.transStart].GetX();
              y2 = coordBuffer->buffer[hnd.transStart].GetY();
          } else {
              x2 = coordBuffer->buffer[hnd.transStart+(hnd.i+1)*hnd.direction].GetX();
              y2 = coordBuffer->buffer[hnd.transStart+(hnd.i+1)*hnd.direction].GetY();
          }
          deltaX = (x2 - x);
          deltaY = (y2 - y);
          len = sqrt(deltaX*deltaX + deltaY*deltaY);

          fracToGo = l/len;
          if(fracToGo <= 1.0) {
              origin.Set(x + deltaX*fracToGo,y + deltaY*fracToGo);
              return true;
          }

          //advance to next point on the path
          l -= len;
          x = x2;
          y = y2;
          hnd.i++;
      }
      return false;
  }

  void MapPainterQt::DrawContourSymbol(const Projection& projection,
                                       const MapParameter& parameter,
                                       const Symbol& symbol,
                                       double space,
                                       size_t transStart,
                                       size_t transEnd)
  {
      double minX;
      double minY;
      double maxX;
      double maxY;

      symbol.GetBoundingBox(minX,minY,maxX,maxY);

      double widthPx=ConvertWidthToPixel(parameter,maxX-minX);
      double height=maxY-minY;

      bool isClosed = false;
      Vertex2D origin;
      double x1,y1,x2,y2,x3,y3,slope;
      FollowPathHandle followPathHnd;
      followPathInit(followPathHnd, origin, transStart, transEnd, isClosed, true);
      if(!isClosed && !followPath(followPathHnd, space/2, origin)){
              return;
      }
      QTransform savedTransform = painter->transform();
      QTransform t;
      bool loop = true;
      while (loop){
          x1 = origin.GetX();
          y1 = origin.GetY();
          loop = followPath(followPathHnd, widthPx/2, origin);
          if(loop){
              x2 = origin.GetX();
              y2 = origin.GetY();
              loop = followPath(followPathHnd, widthPx/2, origin);
              if(loop){
                  x3 = origin.GetX();
                  y3 = origin.GetY();
                  slope = atan2(y3-y1,x3-x1);
                  t = QTransform::fromTranslate(x2, y2);
                  t.rotateRadians(slope);
                  painter->setTransform(t);
                  DrawSymbol(projection, parameter, symbol, 0, height);
                  loop = followPath(followPathHnd, space, origin);
              }
           }
      }
      painter->setTransform(savedTransform);
  }

  void MapPainterQt::DrawIcon(const IconStyle* style,
                              double x, double y)
  {
    size_t idx=style->GetIconId()-1;

    assert(idx<images.size());
    assert(!images[idx].isNull());

    painter->drawImage(QPointF(x-images[idx].width()/2,
                               y-images[idx].height()/2),
                       images[idx]);
  }

  void MapPainterQt::DrawSymbol(const Projection& projection,
                                const MapParameter& parameter,
                                const Symbol& symbol,
                                double x, double y)
  {
    double minX;
    double minY;
    double maxX;
    double maxY;
    double centerX;
    double centerY;

    symbol.GetBoundingBox(minX,minY,maxX,maxY);

    centerX=maxX-minX;
    centerY=maxY-minY;

    for (std::list<DrawPrimitiveRef>::const_iterator p=symbol.GetPrimitives().begin();
         p!=symbol.GetPrimitives().end();
         ++p) {
      DrawPrimitive* primitive=p->Get();

      if (dynamic_cast<PolygonPrimitive*>(primitive)!=NULL) {
        PolygonPrimitive* polygon=dynamic_cast<PolygonPrimitive*>(primitive);
        FillStyleRef      style=polygon->GetFillStyle();

        SetFill(projection,
                parameter,
                *style);

        QPainterPath path;

        for (std::list<Coord>::const_iterator pixel=polygon->GetCoords().begin();
             pixel!=polygon->GetCoords().end();
             ++pixel) {
          if (pixel==polygon->GetCoords().begin()) {
            path.moveTo(x+ConvertWidthToPixel(parameter,pixel->x-centerX),
                        y+ConvertWidthToPixel(parameter,maxY-pixel->y-centerY));
          }
          else {
            path.lineTo(x+ConvertWidthToPixel(parameter,pixel->x-centerX),
                        y+ConvertWidthToPixel(parameter,maxY-pixel->y-centerY));
          }
        }

        painter->drawPath(path);
      }
      else if (dynamic_cast<RectanglePrimitive*>(primitive)!=NULL) {
        RectanglePrimitive* rectangle=dynamic_cast<RectanglePrimitive*>(primitive);
        FillStyleRef        style=rectangle->GetFillStyle();

        SetFill(projection,
                parameter,
                *style);

        QPainterPath path;

        path.addRect(x+ConvertWidthToPixel(parameter,rectangle->GetTopLeft().x-centerX),
                     y+ConvertWidthToPixel(parameter,maxY-rectangle->GetTopLeft().y-centerY),
                     ConvertWidthToPixel(parameter,rectangle->GetWidth()),
                     ConvertWidthToPixel(parameter,rectangle->GetHeight()));

        painter->drawPath(path);
      }
      else if (dynamic_cast<CirclePrimitive*>(primitive)!=NULL) {
        CirclePrimitive* circle=dynamic_cast<CirclePrimitive*>(primitive);
        FillStyleRef     style=circle->GetFillStyle();

        SetFill(projection,
                parameter,
                *style);

        QPainterPath path;

        path.addEllipse(QPointF(x+ConvertWidthToPixel(parameter,circle->GetCenter().x-centerX),
                                y+ConvertWidthToPixel(parameter,maxY-circle->GetCenter().y-centerY)),
                        ConvertWidthToPixel(parameter,circle->GetRadius()),
                        ConvertWidthToPixel(parameter,circle->GetRadius()));

        painter->drawPath(path);
      }
    }
  }

  void MapPainterQt::DrawPath(const Projection& /*projection*/,
                              const MapParameter& /*parameter*/,
                              const Color& color,
                              double width,
                              const std::vector<double>& dash,
                              LineStyle::CapStyle startCap,
                              LineStyle::CapStyle endCap,
                              size_t transStart, size_t transEnd)
  {
    QPen pen;

    pen.setColor(QColor::fromRgbF(color.GetR(),
                                  color.GetG(),
                                  color.GetB(),
                                  color.GetA()));
    pen.setWidthF(width);
    pen.setJoinStyle(Qt::RoundJoin);

   if (startCap==LineStyle::capButt ||
       endCap==LineStyle::capButt) {
      pen.setCapStyle(Qt::FlatCap);
    }
    else if (startCap==LineStyle::capSquare ||
             endCap==LineStyle::capSquare) {
      pen.setCapStyle(Qt::SquareCap);
    }
    else {
      pen.setCapStyle(Qt::RoundCap);
    }

    if (dash.empty()) {
      pen.setStyle(Qt::SolidLine);
    }
    else {
      QVector<qreal> dashes;

      for (size_t i=0; i<dash.size(); i++) {
        dashes << dash[i];
      }

      pen.setDashPattern(dashes);
    }

/*
    painter->setPen(pen);
    size_t last=0;
    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          start=false;
        }
        else {
          painter->drawLine(QPointF(nodeX[last],nodeY[last]),QPointF(nodeX[i],nodeY[i]));
        }

        last=i;
      }
    }*/

    QPainterPath p;

    p.moveTo(coordBuffer->buffer[transStart].GetX(),
             coordBuffer->buffer[transStart].GetY());
    for (size_t i=transStart+1; i<=transEnd; i++) {
      p.lineTo(coordBuffer->buffer[i].GetX(),
               coordBuffer->buffer[i].GetY());
    }

    painter->strokePath(p,pen);
/*
    QPolygonF polygon;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        polygon << QPointF(nodeX[i],nodeY[i]);
      }
    }

    painter->setPen(pen);
    painter->drawPolyline(polygon);*/

    if (dash.empty() &&
        startCap==LineStyle::capRound &&
        endCap!=LineStyle::capRound) {
      painter->setBrush(QBrush(QColor::fromRgbF(color.GetR(),
                                                color.GetG(),
                                                color.GetB(),
                                                color.GetA())));

      painter->drawEllipse(QPointF(coordBuffer->buffer[transStart].GetX(),
                                   coordBuffer->buffer[transStart].GetY()),
                                   width/2,width/2);
    }

    if (dash.empty() &&
      endCap==LineStyle::capRound &&
      startCap!=LineStyle::capRound) {
      painter->setBrush(QBrush(QColor::fromRgbF(color.GetR(),
                                                color.GetG(),
                                                color.GetB(),
                                                color.GetA())));

      painter->drawEllipse(QPointF(coordBuffer->buffer[transEnd].GetX(),
                                   coordBuffer->buffer[transEnd].GetY()),
                                   width/2,width/2);
    }
  }

  void MapPainterQt::DrawArea(const Projection& projection,
                              const MapParameter& parameter,
                              const MapPainter::AreaData& area)
  {
    QPainterPath path;

    path.moveTo(coordBuffer->buffer[area.transStart].GetX(),
                coordBuffer->buffer[area.transStart].GetY());
    for (size_t i=area.transStart+1; i<=area.transEnd; i++) {
      path.lineTo(coordBuffer->buffer[i].GetX(),
                  coordBuffer->buffer[i].GetY());
    }
    path.closeSubpath();

    if (!area.clippings.empty()) {
      for (std::list<PolyData>::const_iterator c=area.clippings.begin();
          c!=area.clippings.end();
          c++) {
        const PolyData& data=*c;

        path.moveTo(coordBuffer->buffer[data.transStart].GetX(),
                    coordBuffer->buffer[data.transStart].GetY());
        for (size_t i=data.transStart+1; i<=data.transEnd; i++) {
          path.lineTo(coordBuffer->buffer[i].GetX(),
                      coordBuffer->buffer[i].GetY());
        }
        path.closeSubpath();
      }
    }

    SetFill(projection,
            parameter,
            *area.fillStyle);
    bool restoreTransform = false;
    size_t idx = -1;
    if(area.fillStyle->HasPattern()){
        idx=area.fillStyle->GetPatternId()-1;
        if(idx<patterns.size() && !patterns[idx].textureImage().isNull()){
            patterns[idx].setTransform(QTransform::fromTranslate(
                                              remainder(coordBuffer->buffer[area.transStart].GetX(),patterns[idx].textureImage().width()),
                                              remainder(coordBuffer->buffer[area.transStart].GetY(),patterns[idx].textureImage().height())));
            painter->setBrush(patterns[idx]);
            restoreTransform = true;
        }
    }
    painter->drawPath(path);
    if(restoreTransform){
        patterns[idx].setTransform(QTransform(1.0,0.0,1.0,0.0,0.0,0.0));
    }
  }

  void MapPainterQt::DrawGround(const Projection& projection,
                                const MapParameter& /*parameter*/,
                                const FillStyle& style)
  {
    painter->fillRect(QRectF(0,0,projection.GetWidth(),projection.GetHeight()),
                      QBrush(QColor::fromRgbF(style.GetFillColor().GetR(),
                                              style.GetFillColor().GetG(),
                                              style.GetFillColor().GetB(),
                                              1)));
  }

  void MapPainterQt::SetPen(const LineStyle& style,
                            double lineWidth)
  {
    QPen pen;

    pen.setColor(QColor::fromRgbF(style.GetLineColor().GetR(),
                                  style.GetLineColor().GetG(),
                                  style.GetLineColor().GetB(),
                                  style.GetLineColor().GetA()));
    pen.setWidthF(lineWidth);

    if (style.GetDash().empty()) {
      pen.setStyle(Qt::SolidLine);
      pen.setCapStyle(Qt::RoundCap);
    }
    else {
      QVector<qreal> dashes;

      for (size_t i=0; i<style.GetDash().size(); i++) {
        dashes << style.GetDash()[i];
      }

      pen.setDashPattern(dashes);
      pen.setCapStyle(Qt::FlatCap);
    }

    painter->setPen(pen);
  }

  void MapPainterQt::SetFill(const Projection& projection,
                             const MapParameter& parameter,
                             const FillStyle& fillStyle)
  {
    double borderWidth=ConvertWidthToPixel(parameter,
                                           fillStyle.GetBorderWidth());

    if (fillStyle.HasPattern() &&
        projection.GetMagnification()>=fillStyle.GetPatternMinMag() &&
        HasPattern(parameter,fillStyle)) {
      size_t idx=fillStyle.GetPatternId()-1;

      painter->setBrush(patterns[idx]);
    }
    else if (fillStyle.GetFillColor().IsVisible()) {
      painter->setBrush(QBrush(QColor::fromRgbF(fillStyle.GetFillColor().GetR(),
                                                fillStyle.GetFillColor().GetG(),
                                                fillStyle.GetFillColor().GetB(),
                                                fillStyle.GetFillColor().GetA())));
    }
    else {
      painter->setBrush(Qt::NoBrush);
    }

    if (borderWidth>=parameter.GetLineMinWidthPixel()) {
      QPen pen;

      pen.setColor(QColor::fromRgbF(fillStyle.GetBorderColor().GetR(),
                                    fillStyle.GetBorderColor().GetG(),
                                    fillStyle.GetBorderColor().GetB(),
                                    fillStyle.GetBorderColor().GetA()));
      pen.setWidthF(borderWidth);

      if (fillStyle.GetBorderDash().empty()) {
        pen.setStyle(Qt::SolidLine);
        pen.setCapStyle(Qt::RoundCap);
      }
      else {
        QVector<qreal> dashes;

        for (size_t i=0; i<fillStyle.GetBorderDash().size(); i++) {
          dashes << fillStyle.GetBorderDash()[i];
        }

        pen.setDashPattern(dashes);
        pen.setCapStyle(Qt::FlatCap);
      }

      painter->setPen(pen);
    }
    else {
      painter->setPen(Qt::NoPen);
    }

  }

  bool MapPainterQt::DrawMap(const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& data,
                             QPainter* painter)
  {
    this->painter=painter;

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    Draw(projection,
         parameter,
         data);

    return true;
  }
}

