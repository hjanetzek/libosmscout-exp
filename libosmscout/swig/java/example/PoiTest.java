import static java.lang.System.out;

import org.osmscout.AreaRef;
import org.osmscout.Areas;
import org.osmscout.Database;
import org.osmscout.DatabaseParameter;
import org.osmscout.DatabaseRef;
import org.osmscout.NodeRef;
import org.osmscout.Nodes;
import org.osmscout.POIService;
import org.osmscout.TypeConfigRef;
import org.osmscout.TypeSet;
import org.osmscout.WayRef;
import org.osmscout.Ways;

public class PoiTest {
	public static void main(String[] args) {
		System.loadLibrary("osmscout_jni");

		DatabaseParameter params = new DatabaseParameter();

		DatabaseRef db = Database.createDatabase(params);

		boolean ok = db.open("/home/jeff/dev/libosmscout/maps/bremen-latest");
		out.println("ok: " + ok);

		POIService service = new POIService(db);
		double lonMin = 8.7;
		double latMin = 52.0;
		double lonMax = 8.9;
		double latMax = 53.1;

		TypeConfigRef typeConfig = db.getTypeConfig();

		args = new String[] { "amenity_bar", "amenity_hospital", "amenity_hospital_building" };

		TypeSet types = new TypeSet(typeConfig.get());
		for (String typename : args) {
			int nodeId = typeConfig.getNodeTypeId(typename);
			int areaId = typeConfig.getAreaTypeId(typename);
			int wayId = typeConfig.getWayTypeId(typename);
			if (nodeId == 0 && wayId == 0 && areaId == 0) { // osmscout.getTypeIgnore())
				out.println("ignored type " + typename);
				continue;
			}
			out.println("w:" + wayId + " a:" + areaId + " n:" + nodeId);

			if (nodeId != 0)
				types.setType(nodeId);

			if (wayId != 0)
				types.setType(wayId);

			if (areaId != 0)
				types.setType(areaId);

		}
		// TypeInfos typeInfos = typeConfig.getTypes();
		// typeInfos.

		Ways ways = new Ways();
		Areas areas = new Areas();
		Nodes nodes = new Nodes();

		service.getPOIsInArea(
				lonMin, latMin,
				lonMax, latMax,
				types,
				nodes, ways, areas);

		NodeRef node = new NodeRef();
		WayRef way = new WayRef();
		AreaRef area = new AreaRef();

		for (int i = 0, n = nodes.size(); i < n; i++) {
			nodes.get(i, node);
			out.println("node: " + node.getName());
		}

		for (int i = 0, n = ways.size(); i < n; i++) {
			ways.get(i, way);
			out.println("way: " + way.getName());
		}
		for (int i = 0, n = areas.size(); i < n; i++) {
			areas.get(i, area);
			out.println("area: " + area.getRing(0).getName());
		}

		node.delete();
		way.delete();
		area.delete();
	}
}
