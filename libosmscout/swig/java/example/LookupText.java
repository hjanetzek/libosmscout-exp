import static java.lang.System.out;

import org.osmscout.AreaRef;
import org.osmscout.Database;
import org.osmscout.DatabaseParameter;
import org.osmscout.NodeRef;
import org.osmscout.ObjectFileRef;
import org.osmscout.ObjectFileRefs;
import org.osmscout.RefType;
import org.osmscout.TextSearchIndex;
import org.osmscout.TextSearchResults;
import org.osmscout.WayRef;

public class LookupText {
	public static void main(String[] args) {

		System.loadLibrary("osmscout_jni");

		String path = "/home/jeff/dev/libosmscout/maps/bremen-latest";
		// String query = "Unique";
		String query = "erlenstraße";
		// String query = "Universität";

		// String query = "Erlenstraße";

		DatabaseParameter p = new DatabaseParameter();

		Database db = new Database(p);

		if (!db.open(path)) {
			out.println("could not open db: " + path);
		}

		TextSearchIndex textSearch = new TextSearchIndex();
		if (!textSearch.load(path)) {
			out.println("could not load: " + path);
			return;
		}

		TextSearchResults results = new TextSearchResults();

		if (!textSearch.search(query, true, true, true, true, results)) {
			out.println("...");
		}

		out.println("results: " + results.size());

		NodeRef node = new NodeRef();
		WayRef way = new WayRef();
		AreaRef area = new AreaRef();

		for (String name : results) {
			out.println("name: " + name);
			ObjectFileRefs refs = results.get(name);
			for (int i = 0, n = refs.size(); i < n; i++) {
				ObjectFileRef r = refs.get(0);
				int offset = r.getFileOffset();
				out.println(r.getTypeName() + " " + offset);

				RefType type = r.getType();

				if (type == RefType.refNode) {
					db.getNodeByOffset(offset, node);
					out.println(">: " + node.getName() + "/" + node.getAddress()); // +
																					// "/"
																					// +
																					// node.getLocation());
				} else if (type == RefType.refWay) {
					db.getWayByOffset(offset, way);
					if (!way.invalid())
						out.println("> " + way.getName());
				} else if (type == RefType.refArea) {
					db.getAreaByOffset(offset, area);
					if (!area.invalid())
						out.println("> " + area.getRing(0).getName());

				}
			}
		}
	}
}
