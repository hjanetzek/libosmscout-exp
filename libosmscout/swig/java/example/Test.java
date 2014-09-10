import static java.lang.System.out;

import org.osmscout.AreaRef;
import org.osmscout.AreaSearchParameter;
import org.osmscout.Areas;
import org.osmscout.Database;
import org.osmscout.DatabaseParameter;
import org.osmscout.DatabaseRef;
import org.osmscout.GeoCoord;
import org.osmscout.GeoCoords;
import org.osmscout.Magnification;
import org.osmscout.MapService;
import org.osmscout.Nodes;
import org.osmscout.TagInfo;
import org.osmscout.TagInfos;
import org.osmscout.TypeConfigRef;
import org.osmscout.TypeInfo;
import org.osmscout.TypeInfos;
import org.osmscout.TypeSet;
import org.osmscout.TypeSets;
import org.osmscout.WayRef;
import org.osmscout.Ways;

public class Test {
	public static void main(String[] args) {

		System.loadLibrary("osmscout_jni");

		DatabaseParameter p = new DatabaseParameter();
		Database b = new Database(p);

		boolean ok = b.open("/home/jeff/dev/libosmscout/maps/bremen-latest");
		out.println("ok: " + ok);
		Ways ways = new Ways();
		Areas areas = new Areas();
		Nodes nodes = new Nodes();

		Magnification magnification = new Magnification(1 << 15);
		AreaSearchParameter parameter = new AreaSearchParameter();

		out.println("max: " + parameter.getMaximumNodes() + " "
				+ parameter.getMaximumAreas() + " " + parameter.getMaximumWays() + " "
				);

		parameter.setMaximumAreas(100);

		TypeSet type = new TypeSet();
		TypeConfigRef typeConfig = b.getTypeConfig();

		TypeInfos typeInfos = typeConfig.getTypes();
		String[] typeMap = new String[typeInfos.size()];

		for (int i = 0, n = typeInfos.size(); i < n; i++) {
			TypeInfo info = typeInfos.get(i);
			// System.out.println("type: " + info.getId() + " " +
			// info.getName());
			typeMap[i] = makeTag(info.getName());

			out.println(info.getId() + ":" + typeMap[i]);

			type.setType((short) info.getId());
		}

		TagInfos tags = typeConfig.getTags();
		for (int i = 0, n = tags.size(); i < n; i++) {
			TagInfo info = tags.get(i);
			out.println("tags: " + info.getId() + " " + info.getName());

		}
		TypeSets types = new TypeSets();
		types.pushBack(type);

		MapService s = new MapService(new DatabaseRef(b));
		ok = s.getObjects(
				type, types, type, 8.8, 53.0, 8.81, 53.01, magnification, parameter,
				nodes, ways, areas);

		out.println("ok: " + ok);

		out.println("ways: " + ways.size());
		out.println("nodes: " + nodes.size());
		out.println("areas: " + areas.size());

		for (int i = 0, n = (int) areas.size(); i < n; i++) {
			AreaRef area = areas.get(i);
			String name = typeMap[area.getType()] + "\n\t" + area.getRing(0).getAttributes().getName();
			out.println("area: " + name);

		}

		for (int i = 0, n = (int) ways.size(); i < n; i++) {
			WayRef way = ways.get(i);
			String name = way.getTagCount() + "\n\t" + typeMap[way.getType()] + " " + way.getName() + " "
					+ way.getRefName();
			out.println("way: " + name);

			GeoCoords coords = way.getNodes();
			for (int j = 0, m = coords.size(); j < m; j++) {
				GeoCoord c = coords.get(j);
				out.print(String.format("%.4f,%.4f; ", c.getLat(), c.getLon()));
				// + " " + nodes[i * 2] + " " + nodes[i * 2 + 1]
			}
			out.println();
		}

		b.close();

	}

	public static String makeTag(String s) {
		int sep;
		String tag;
		if (s.startsWith("man_made"))
			sep = 8;
		else
			sep = s.indexOf('_');

		if (sep > 0) {
			int sep2 = s.indexOf("building", sep + 1);
			if (sep2 > 0 && sep2 > sep + 1) {

				tag = s.substring(0, sep) + "/" + s.substring(sep + 1, sep2 - 1);
			} else {
				tag = s.substring(0, sep) + "/" + s.substring(sep + 1);
			}
			// return new Tag(s.substring(0, sep), s.substring(sep + 1));
		} else {
			tag = s + " /yes";
		}
		return tag;
	}

}
