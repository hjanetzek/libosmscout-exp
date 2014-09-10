import static java.lang.System.out;

import java.util.Arrays;

import org.osmscout.GeoCoord;
import org.osmscout.GeoCoords;
import org.osmscout.Magnification;
import org.osmscout.MercatorProjection;
import org.osmscout.Transformer;
import org.osmscout.Way;

public class TransformerTest {
	static {
		System.loadLibrary("osmscout_java");
	}

	public static void main(String[] args) {

		Way way = new Way();
		GeoCoords coords = new GeoCoords();
		double lat = 85.05112877980659;
		coords.pushBack(new GeoCoord(-lat, -180));
		coords.pushBack(new GeoCoord(-lat, 180));
		coords.pushBack(new GeoCoord(lat, -180));
		coords.pushBack(new GeoCoord(lat, 180));
		coords.pushBack(new GeoCoord(0, 0));

		for (int i = 1; i < 10; i++) {
			coords.pushBack(new GeoCoord(i, i));
		}

		way.setNodes(coords);

		Transformer t = new Transformer();
		t.setProjection(0, 0, 1, 1024);
		int[] start = { 0 };
		int[] end = { 0 };

		t.transformWay(way, start, end);
		out.println(start[0] + " " + end[0]);
		float[] points = new float[20];
		out.println(Arrays.toString(points));

		t.copyRange(0, 10, points);

		out.println(Arrays.toString(points));

		MercatorProjection p = new MercatorProjection();
		p.set(-180, -lat, 180, lat, new Magnification(1), 1024);
		double[] x = { 0 };
		double[] y = { 0 };
		p.geoToPixel(-180, lat, x, y);
		out.println(x[0] + " " + y[0]);

	}
}
