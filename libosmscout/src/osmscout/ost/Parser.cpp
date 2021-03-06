/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

#include <osmscout/ost/Parser.h>

#include <osmscout/ost/Scanner.h>

#include <iostream>
#include <sstream>

namespace osmscout {
namespace ost {


void Parser::SynErr(int n)
{
  if (errDist >= minErrDist) {
    errors->SynErr(la->line, la->col, n);
  }
  errDist = 0;
}

void Parser::SemErr(const char* msg)
{
  if (errDist >= minErrDist) {
    errors->Error(t->line, t->col, msg);
  }
  errDist = 0;
}

void Parser::Get()
{
  for (;;) {
    t = la;
    la = scanner->Scan();
    if (la->kind <= maxT) {
      ++errDist;
      break;
    }

    if (dummyToken != t) {
      dummyToken->kind = t->kind;
      dummyToken->pos = t->pos;
      dummyToken->col = t->col;
      dummyToken->line = t->line;
      dummyToken->next = NULL;
      coco_string_delete(dummyToken->val);
      dummyToken->val = coco_string_create(t->val);
      t = dummyToken;
    }
    la = t;
  }
}

void Parser::Expect(int n)
{
  if (la->kind==n) {
    Get();
  }
  else {
    SynErr(n);
  }
}

void Parser::ExpectWeak(int n, int follow)
{
  if (la->kind == n) {
    Get();
  }
  else {
    SynErr(n);
    while (!StartOf(follow)) {
      Get();
    }
  }
}

bool Parser::WeakSeparator(int n, int syFol, int repFol)
{
  if (la->kind == n) {
    Get();
    return true;
  }
  else if (StartOf(repFol)) {
    return false;
  }
  else {
    SynErr(n);
    while (!(StartOf(syFol) || StartOf(repFol) || StartOf(0))) {
      Get();
    }
    return StartOf(syFol);
  }
}

void Parser::OST() {
		while (!(la->kind == _EOF || la->kind == 4 /* "OST" */)) {SynErr(52); Get();}
		Expect(4 /* "OST" */);
		if (la->kind == 6 /* "MAX" */) {
			MAXSPEEDS();
		}
		if (la->kind == 11 /* "GRADES" */) {
			GRADES();
		}
		if (la->kind == 16 /* "TYPES" */) {
			TYPES();
		}
		Expect(5 /* "END" */);
}

void Parser::MAXSPEEDS() {
		while (!(la->kind == _EOF || la->kind == 6 /* "MAX" */)) {SynErr(53); Get();}
		Expect(6 /* "MAX" */);
		Expect(7 /* "SPEEDS" */);
		MAXSPEED();
		while (la->kind == 8 /* "SPEED" */) {
			MAXSPEED();
		}
}

void Parser::GRADES() {
		while (!(la->kind == _EOF || la->kind == 11 /* "GRADES" */)) {SynErr(54); Get();}
		Expect(11 /* "GRADES" */);
		GRADE();
		while (la->kind == 12 /* "SURFACE" */) {
			GRADE();
		}
}

void Parser::TYPES() {
		while (!(la->kind == _EOF || la->kind == 16 /* "TYPES" */)) {SynErr(55); Get();}
		Expect(16 /* "TYPES" */);
		TYPE();
		while (la->kind == 17 /* "TYPE" */) {
			TYPE();
		}
}

void Parser::MAXSPEED() {
		std::string alias;
		size_t      speed; 
		
		while (!(la->kind == _EOF || la->kind == 8 /* "SPEED" */)) {SynErr(56); Get();}
		Expect(8 /* "SPEED" */);
		STRING(alias);
		Expect(9 /* "=" */);
		UINT(speed);
		if (speed>0 && speed<256) {
		 config.RegisterMaxSpeedAlias(alias,
		                              (uint8_t)speed);
		}
		else {
		    std::string e="Speed value not in the allowed range (]0..255] km/h)";
		
		    SemErr(e.c_str());
		}
		
		if (la->kind == 10 /* "km/h" */) {
			Get();
		}
}

void Parser::STRING(std::string& value) {
		Expect(_string);
		value=Destring(t->val);
		
}

void Parser::UINT(size_t& value) {
		Expect(_number);
		if (!StringToNumber(t->val,value)) {
		 std::string e="Cannot parse number '"+std::string(t->val)+"'";
		
		 SemErr(e.c_str());
		}
		
}

void Parser::GRADE() {
		size_t grade;
		
		while (!(la->kind == _EOF || la->kind == 12 /* "SURFACE" */)) {SynErr(57); Get();}
		Expect(12 /* "SURFACE" */);
		Expect(13 /* "GRADE" */);
		UINT(grade);
		Expect(14 /* "{" */);
		while (la->kind == _string) {
			std::string surface;
			
			STRING(surface);
			if (grade>=1 && grade<=5) {
			 config.RegisterSurfaceToGradeMapping(surface,
			                                      grade);
			}
			else {
			 std::string e="Not a valid grade level: "+NumberToString(grade);
			
			 SemErr(e.c_str());
			}
			
		}
		Expect(15 /* "}" */);
}

void Parser::TYPE() {
		std::string   name;
		TagCondition  *condition=NULL;
		TypeInfoRef   typeInfo(new TypeInfo());
		unsigned char types;
		
		while (!(la->kind == _EOF || la->kind == 17 /* "TYPE" */)) {SynErr(58); Get();}
		Expect(17 /* "TYPE" */);
		IDENT(name);
		typeInfo->SetType(name); 
		if (la->kind == 18 /* "IGNORE" */) {
			Get();
			typeInfo->SetIgnore(true); 
		}
		Expect(9 /* "=" */);
		TYPEKINDS(types);
		Expect(19 /* "(" */);
		TAGCONDITION(condition);
		Expect(20 /* ")" */);
		typeInfo->AddCondition(types,condition); 
		while (la->kind == 21 /* "OR" */) {
			Get();
			TYPEKINDS(types);
			Expect(19 /* "(" */);
			TAGCONDITION(condition);
			Expect(20 /* ")" */);
			typeInfo->AddCondition(types,condition); 
		}
		if (la->kind == 14 /* "{" */) {
			Get();
			if (la->kind == _ident) {
				FEATURE(*typeInfo);
				while (la->kind == 22 /* "," */) {
					Get();
					FEATURE(*typeInfo);
				}
			}
			Expect(15 /* "}" */);
		}
		if (StartOf(1)) {
			TYPEOPTIONS(*typeInfo);
		}
		config.RegisterType(typeInfo);
		
}

void Parser::IDENT(std::string& value) {
		Expect(_ident);
		value=t->val;
		
}

void Parser::TYPEKINDS(unsigned char& types) {
		types=0;
		
		TYPEKIND(types);
		while (StartOf(2)) {
			if (la->kind == 22 /* "," */) {
				Get();
			}
			TYPEKIND(types);
		}
}

void Parser::TAGCONDITION(TagCondition*& condition) {
		std::list<TagCondition*> conditions;
		TagCondition             *subCond;
		
		TAGANDCOND(subCond);
		conditions.push_back(subCond); 
		while (la->kind == 21 /* "OR" */) {
			Get();
			TAGANDCOND(subCond);
			conditions.push_back(subCond); 
		}
		if (conditions.size()==1) {
		 condition=conditions.front();
		}
		else {
		 TagBoolCondition *orCondition=new TagBoolCondition(TagBoolCondition::boolOr);
		
		 for (std::list<TagCondition*>::const_iterator c=conditions.begin();
		      c!=conditions.end();
		      ++c) {
		   orCondition->AddCondition(*c);
		 }
		
		 condition=orCondition;
		}
		
}

void Parser::FEATURE(TypeInfo& typeInfo) {
		std::string name; 
		IDENT(name);
		FeatureRef feature=config.GetFeature(name);
		
		if (feature.Valid()) {
		 if (!typeInfo.HasFeature(name)) {
		   typeInfo.AddFeature(feature);
		 }
		 else {
		 std::string e="Feature '"+name+"' has been already assigned to type";
		
		 SemErr(e.c_str());
		 }
		}
		else {
		 std::string e="Feature '"+name+"' is unknown";
		
		 SemErr(e.c_str());
		}                
		
}

void Parser::TYPEOPTIONS(TypeInfo& typeInfo) {
		TYPEOPTION(typeInfo);
		while (StartOf(1)) {
			TYPEOPTION(typeInfo);
		}
}

void Parser::TAGANDCOND(TagCondition*& condition) {
		std::list<TagCondition*> conditions;
		TagCondition             *subCond;
		
		TAGBOOLCOND(subCond);
		conditions.push_back(subCond); 
		while (la->kind == 23 /* "AND" */) {
			Get();
			TAGBOOLCOND(subCond);
			conditions.push_back(subCond); 
		}
		if (conditions.size()==1) {
		 condition=conditions.front();
		}
		else {
		 TagBoolCondition *andCondition=new TagBoolCondition(TagBoolCondition::boolAnd);
		
		 for (std::list<TagCondition*>::const_iterator c=conditions.begin();
		      c!=conditions.end();
		      ++c) {
		   andCondition->AddCondition(*c);
		 }
		
		 condition=andCondition;
		}
		
}

void Parser::TAGBOOLCOND(TagCondition*& condition) {
		if (la->kind == _string) {
			TAGBINCOND(condition);
		} else if (la->kind == 34 /* "EXISTS" */) {
			TAGEXISTSCOND(condition);
		} else if (la->kind == 19 /* "(" */) {
			Get();
			TAGCONDITION(condition);
			Expect(20 /* ")" */);
		} else if (la->kind == 24 /* "!" */) {
			Get();
			TAGBOOLCOND(condition);
			condition=new TagNotCondition(condition); 
		} else SynErr(59);
}

void Parser::TAGBINCOND(TagCondition*& condition) {
		std::string nameValue;
		
		Expect(_string);
		nameValue=Destring(t->val); 
		switch (la->kind) {
		case 25 /* "<" */: {
			TAGLESSCOND(nameValue,condition);
			break;
		}
		case 26 /* "<=" */: {
			TAGLESSEQUALCOND(nameValue,condition);
			break;
		}
		case 27 /* "==" */: {
			TAGEQUALSCOND(nameValue,condition);
			break;
		}
		case 28 /* "!=" */: {
			TAGNOTEQUALSCOND(nameValue,condition);
			break;
		}
		case 30 /* ">" */: {
			TAGGREATERCOND(nameValue,condition);
			break;
		}
		case 29 /* ">=" */: {
			TAGGREATEREQUALCOND(nameValue,condition);
			break;
		}
		case 31 /* "IN" */: {
			TAGISINCOND(nameValue,condition);
			break;
		}
		default: SynErr(60); break;
		}
}

void Parser::TAGEXISTSCOND(TagCondition*& condition) {
		Expect(34 /* "EXISTS" */);
		Expect(_string);
		condition=new TagExistsCondition(config.RegisterTag(Destring(t->val)));
		
}

void Parser::TAGLESSCOND(const std::string& tagName,TagCondition*& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(25 /* "<" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorLess,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorLess,sizeValue);
			
		} else SynErr(61);
}

void Parser::TAGLESSEQUALCOND(const std::string& tagName,TagCondition*& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(26 /* "<=" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorLessEqual,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorLessEqual,sizeValue);
			
		} else SynErr(62);
}

void Parser::TAGEQUALSCOND(const std::string& tagName,TagCondition*& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(27 /* "==" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorEqual,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorEqual,sizeValue);
			
		} else SynErr(63);
}

void Parser::TAGNOTEQUALSCOND(const std::string& tagName,TagCondition*& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(28 /* "!=" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorNotEqual,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorNotEqual,sizeValue);
			
		} else SynErr(64);
}

void Parser::TAGGREATERCOND(const std::string& tagName,TagCondition*& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(30 /* ">" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorGreater,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorGreater,sizeValue);
			
		} else SynErr(65);
}

void Parser::TAGGREATEREQUALCOND(const std::string& tagName,TagCondition*& condition) {
		std::string stringValue;
		size_t      sizeValue;
		
		Expect(29 /* ">=" */);
		if (la->kind == _string) {
			STRING(stringValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorGreaterEqual,stringValue);
			
		} else if (la->kind == _number) {
			UINT(sizeValue);
			TagId tagId=config.RegisterTag(tagName);
			
			condition=new TagBinaryCondition(tagId,operatorGreaterEqual,sizeValue);
			
		} else SynErr(66);
}

void Parser::TAGISINCOND(const std::string& tagName,TagCondition*& condition) {
		std::list<std::string> values;
		
		Expect(31 /* "IN" */);
		Expect(32 /* "[" */);
		Expect(_string);
		values.push_back(Destring(t->val)); 
		while (la->kind == 22 /* "," */) {
			Get();
			Expect(_string);
			values.push_back(Destring(t->val)); 
		}
		Expect(33 /* "]" */);
		TagId tagId=config.RegisterTag(tagName);
		
		if (values.size()==1) {
		 condition=new TagBinaryCondition(tagId,operatorEqual,values.front());
		}
		else {
		 TagIsInCondition *isInCondition=new TagIsInCondition(tagId);
		
		 for (std::list<std::string>::const_iterator s=values.begin();
		      s!=values.end();
		      ++s) {
		   isInCondition->AddTagValue(*s);
		 }
		
		 condition=isInCondition;
		}
		
}

void Parser::TYPEKIND(unsigned char& types) {
		if (la->kind == 35 /* "NODE" */) {
			Get();
			types|=TypeInfo::typeNode; 
		} else if (la->kind == 36 /* "WAY" */) {
			Get();
			types|=TypeInfo::typeWay; 
		} else if (la->kind == 37 /* "AREA" */) {
			Get();
			types|=TypeInfo::typeArea; 
		} else if (la->kind == 38 /* "RELATION" */) {
			Get();
			types|=TypeInfo::typeRelation; 
		} else SynErr(67);
}

void Parser::TYPEOPTION(TypeInfo& typeInfo) {
		switch (la->kind) {
		case 47 /* "PATH" */: {
			PATH(typeInfo);
			break;
		}
		case 39 /* "LOCATION" */: {
			Get();
			typeInfo.SetIndexAsLocation(true); 
			break;
		}
		case 40 /* "ADMIN_REGION" */: {
			Get();
			typeInfo.SetIndexAsRegion(true); 
			break;
		}
		case 41 /* "ADDRESS" */: {
			Get();
			typeInfo.SetIndexAsAddress(true); 
			break;
		}
		case 42 /* "POI" */: {
			Get();
			typeInfo.SetIndexAsPOI(true); 
			break;
		}
		case 43 /* "OPTIMIZE_LOW_ZOOM" */: {
			Get();
			typeInfo.SetOptimizeLowZoom(true); 
			break;
		}
		case 44 /* "MULTIPOLYGON" */: {
			Get();
			typeInfo.SetMultipolygon(true); 
			break;
		}
		case 45 /* "PIN_WAY" */: {
			Get();
			typeInfo.SetPinWay(true); 
			break;
		}
		case 46 /* "IGNORESEALAND" */: {
			Get();
			typeInfo.SetIgnoreSeaLand(true); 
			break;
		}
		default: SynErr(68); break;
		}
}

void Parser::PATH(TypeInfo& typeInfo) {
		Expect(47 /* "PATH" */);
		typeInfo.SetIsPath(true); 
		if (la->kind == 32 /* "[" */) {
			Get();
			if (la->kind == 48 /* "FOOT" */) {
				Get();
				typeInfo.CanRouteFoot(true);
				
			}
			if (la->kind == 49 /* "BICYCLE" */) {
				Get();
				typeInfo.CanRouteBicycle(true);
				
			}
			if (la->kind == 50 /* "CAR" */) {
				Get();
				typeInfo.CanRouteCar(true);
				
			}
			Expect(33 /* "]" */);
		}
}



void Parser::Parse()
{
  t = NULL;
  la = dummyToken = new Token();
  la->val = coco_string_create("Dummy Token");
  Get();
	OST();
	Expect(0);
}

Parser::Parser(Scanner *scanner,
               TypeConfig& config)
 : config(config)
{
	maxT = 51;

  dummyToken = NULL;
  t = la = NULL;
  minErrDist = 2;
  errDist = minErrDist;
  this->scanner = scanner;
  errors = new Errors();
}

bool Parser::StartOf(int s)
{
  const bool T = true;
  const bool x = false;

	static bool set[3][53] = {
		{T,x,x,x, T,x,T,x, T,x,x,T, T,x,x,x, T,T,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,T, T,T,T,T, T,T,T,T, x,x,x,x, x},
		{x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,x,x, x,x,T,x, x,x,x,x, x,x,x,x, x,x,x,T, T,T,T,x, x,x,x,x, x,x,x,x, x,x,x,x, x}
	};



  return set[s][la->kind];
}

Parser::~Parser()
{
  delete errors;
}

Errors::Errors()
 : hasErrors(false)
{
  // no code
}

void Errors::SynErr(int line, int col, int n)
{
  char* s;
  switch (n) {
			case 0: s = coco_string_create("EOF expected"); break;
			case 1: s = coco_string_create("ident expected"); break;
			case 2: s = coco_string_create("number expected"); break;
			case 3: s = coco_string_create("string expected"); break;
			case 4: s = coco_string_create("\"OST\" expected"); break;
			case 5: s = coco_string_create("\"END\" expected"); break;
			case 6: s = coco_string_create("\"MAX\" expected"); break;
			case 7: s = coco_string_create("\"SPEEDS\" expected"); break;
			case 8: s = coco_string_create("\"SPEED\" expected"); break;
			case 9: s = coco_string_create("\"=\" expected"); break;
			case 10: s = coco_string_create("\"km/h\" expected"); break;
			case 11: s = coco_string_create("\"GRADES\" expected"); break;
			case 12: s = coco_string_create("\"SURFACE\" expected"); break;
			case 13: s = coco_string_create("\"GRADE\" expected"); break;
			case 14: s = coco_string_create("\"{\" expected"); break;
			case 15: s = coco_string_create("\"}\" expected"); break;
			case 16: s = coco_string_create("\"TYPES\" expected"); break;
			case 17: s = coco_string_create("\"TYPE\" expected"); break;
			case 18: s = coco_string_create("\"IGNORE\" expected"); break;
			case 19: s = coco_string_create("\"(\" expected"); break;
			case 20: s = coco_string_create("\")\" expected"); break;
			case 21: s = coco_string_create("\"OR\" expected"); break;
			case 22: s = coco_string_create("\",\" expected"); break;
			case 23: s = coco_string_create("\"AND\" expected"); break;
			case 24: s = coco_string_create("\"!\" expected"); break;
			case 25: s = coco_string_create("\"<\" expected"); break;
			case 26: s = coco_string_create("\"<=\" expected"); break;
			case 27: s = coco_string_create("\"==\" expected"); break;
			case 28: s = coco_string_create("\"!=\" expected"); break;
			case 29: s = coco_string_create("\">=\" expected"); break;
			case 30: s = coco_string_create("\">\" expected"); break;
			case 31: s = coco_string_create("\"IN\" expected"); break;
			case 32: s = coco_string_create("\"[\" expected"); break;
			case 33: s = coco_string_create("\"]\" expected"); break;
			case 34: s = coco_string_create("\"EXISTS\" expected"); break;
			case 35: s = coco_string_create("\"NODE\" expected"); break;
			case 36: s = coco_string_create("\"WAY\" expected"); break;
			case 37: s = coco_string_create("\"AREA\" expected"); break;
			case 38: s = coco_string_create("\"RELATION\" expected"); break;
			case 39: s = coco_string_create("\"LOCATION\" expected"); break;
			case 40: s = coco_string_create("\"ADMIN_REGION\" expected"); break;
			case 41: s = coco_string_create("\"ADDRESS\" expected"); break;
			case 42: s = coco_string_create("\"POI\" expected"); break;
			case 43: s = coco_string_create("\"OPTIMIZE_LOW_ZOOM\" expected"); break;
			case 44: s = coco_string_create("\"MULTIPOLYGON\" expected"); break;
			case 45: s = coco_string_create("\"PIN_WAY\" expected"); break;
			case 46: s = coco_string_create("\"IGNORESEALAND\" expected"); break;
			case 47: s = coco_string_create("\"PATH\" expected"); break;
			case 48: s = coco_string_create("\"FOOT\" expected"); break;
			case 49: s = coco_string_create("\"BICYCLE\" expected"); break;
			case 50: s = coco_string_create("\"CAR\" expected"); break;
			case 51: s = coco_string_create("??? expected"); break;
			case 52: s = coco_string_create("this symbol not expected in OST"); break;
			case 53: s = coco_string_create("this symbol not expected in MAXSPEEDS"); break;
			case 54: s = coco_string_create("this symbol not expected in GRADES"); break;
			case 55: s = coco_string_create("this symbol not expected in TYPES"); break;
			case 56: s = coco_string_create("this symbol not expected in MAXSPEED"); break;
			case 57: s = coco_string_create("this symbol not expected in GRADE"); break;
			case 58: s = coco_string_create("this symbol not expected in TYPE"); break;
			case 59: s = coco_string_create("invalid TAGBOOLCOND"); break;
			case 60: s = coco_string_create("invalid TAGBINCOND"); break;
			case 61: s = coco_string_create("invalid TAGLESSCOND"); break;
			case 62: s = coco_string_create("invalid TAGLESSEQUALCOND"); break;
			case 63: s = coco_string_create("invalid TAGEQUALSCOND"); break;
			case 64: s = coco_string_create("invalid TAGNOTEQUALSCOND"); break;
			case 65: s = coco_string_create("invalid TAGGREATERCOND"); break;
			case 66: s = coco_string_create("invalid TAGGREATEREQUALCOND"); break;
			case 67: s = coco_string_create("invalid TYPEKIND"); break;
			case 68: s = coco_string_create("invalid TYPEOPTION"); break;

    default:
    {
      std::stringstream buffer;

      buffer << "error " << n;

      s = coco_string_create(buffer.str().c_str());
    }
    break;
  }

  Err error;

  error.type=Err::Symbol;
  error.line=line;
  error.column=col;
  error.text=s;

  coco_string_delete(s);

  std::cout << error.line << "," << error.column << " " << "Symbol: " << error.text << std::endl;

  errors.push_back(error);
  hasErrors=true;
}

void Errors::Error(int line, int col, const char *s)
{
  Err error;

  error.type=Err::Error;
  error.line=line;
  error.column=col;
  error.text=s;

  std::cout << error.line << "," << error.column << " " << "Error: " << error.text << std::endl;

  errors.push_back(error);
  hasErrors=true;
}

void Errors::Warning(int line, int col, const char *s)
{
  Err error;

  error.type=Err::Warning;
  error.line=line;
  error.column=col;
  error.text=s;

  std::cout << error.line << "," << error.column << " " << "Warning: " << error.text << std::endl;

  errors.push_back(error);
}

void Errors::Warning(const char *s)
{
  Err error;

  error.type=Err::Warning;
  error.line=0;
  error.column=0;
  error.text=s;

  std::cout << error.line << "," << error.column << " " << "Warning: " << error.text << std::endl;

  errors.push_back(error);
}

void Errors::Exception(const char* s)
{
  Err error;

  error.type=Err::Exception;
  error.line=0;
  error.column=0;
  error.text=s;

  std::cout << error.line << "," << error.column << " " << "Exception: " << error.text << std::endl;

  errors.push_back(error);
  hasErrors=true;
}

} // namespace
} // namespace

