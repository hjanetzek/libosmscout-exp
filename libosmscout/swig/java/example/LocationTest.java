import static java.lang.System.out;

import org.osmscout.Address;
import org.osmscout.AdminRegion;
import org.osmscout.AdminRegionRef;
import org.osmscout.Database;
import org.osmscout.DatabaseParameter;
import org.osmscout.DatabaseRef;
import org.osmscout.Location;
import org.osmscout.LocationSearch;
import org.osmscout.LocationSearchEntryList;
import org.osmscout.LocationSearchResult;
import org.osmscout.LocationSearchResultEntryList;
import org.osmscout.LocationService;
import org.osmscout.LocationVisitor;
import org.osmscout.MyVisitor;
import org.osmscout.POI;

public class LocationTest {

	public static void main(String[] args) {

		System.loadLibrary("osmscout_jni");

		DatabaseParameter p = new DatabaseParameter();
		Database b = new Database(p);
		b.addReference();

		boolean ok = b.open("/home/jeff/dev/libosmscout/maps/bremen-latest");
		out.println("ok: " + ok);

		LocationService s = new LocationService(new DatabaseRef(b));
		LocationSearch ls = new LocationSearch();

		// ls.InitializeSearchEntries("Bremen Neustadt Erlenstraße");
		// ls.initializeSearchEntries("Bremen Universität");
		// ls.initializeSearchEntries("Universität");

		LocationSearchEntryList list = new LocationSearchEntryList();
		LocationSearch.Entry entry = new LocationSearch.Entry();

		// entry.setAdminRegionPattern("Bremen");
		entry.setAdminRegionPattern("Horn-Lehe");
		// entry.setLocationPattern("Bibliothekstraße");
		// entry.setLocationPattern("Universität");
		// entry.setLocationPattern("Erlenstraße");
		// entry.setAddressPattern("89");

		list.add(entry);
		ls.setSearches(list);
		ls.setLimit(100);

		LocationSearchResult result = new LocationSearchResult();
		ok = s.searchForLocations(ls, result);
		LocationSearchResultEntryList res = result.getResults();
		out.println("ok search: " + ok + " " + res.size());

		// ObjectFileRef obj = new ObjectFileRef();
		// obj.set(offset, type);

		// AddressVisitor addVisitor = new AddressVisitor() {
		// @Override
		// public boolean visit(AdminRegion adminRegion, Location location,
		// Address address) {
		// out.println(address.getName());
		// return true;
		// }
		// };

		LocationVisitor locVisitor = new LocationVisitor() {

			@Override
			public boolean visit(AdminRegion adminRegion, Location location) {
				out.println("==>" + adminRegion.getName() + " " + adminRegion.getReferenceCount() + " "
						+ location.getName());
				return true;
			}

			@Override
			public boolean visit(AdminRegion adminRegion, POI poi) {

				return true;
			}
		};

		for (LocationSearchResult.Entry e : res) {

			out.println("q : "
					+ "  loc:" + e.getLocationMatchQuality() + " "
					+ "  add:" + e.getAddressMatchQuality() + " "
					+ "  poi:" + e.getPoiMatchQuality() + " "
					+ "  adm:" + e.getAdminRegionMatchQuality());

			Location l = e.getLocation();
			if (l != null) {
				out.println("location : " + l.getName());
			}
			POI poi = e.getPoi();
			if (poi != null) {
				out.println("poi : " + poi.getName());
			}

			Address a = e.getAddress();
			if (a != null) {
				out.println("add : " + a.getName());
			}

			AdminRegion reg = e.getAdminRegion();
			if (reg != null) {
				out.println("reg : " + reg.getName());

				s.visitAdminRegionLocations(reg, locVisitor);
			}

			// if (l != null && reg != null) {
			// s.visitLocationAddresses(reg, l, addVisitor);
			// }
		}

		// out.println('\n');
		// AdminRegionVisitor visitor = new AdminRegionVisitor() {
		//
		// public Action visit(AdminRegion region) {
		// out.println(region.getName() + " : " + region.getAliasName());
		//
		// // region.getObject()
		//
		// return Action.visitChildren;
		// };
		// };

		// s.visitAdminRegions(visitor);
		{
			final AdminRegionRef admref = new AdminRegionRef();
			MyVisitor v = new MyVisitor() {

				@Override
				public Action visit() {
					out.println(":" + admref.getName());
					return Action.visitChildren;
				}
			};
			v.setAdminRegion(admref);
			s.visitAdminRegions(v);

			if (!admref.invalid()) {
				admref.delete();
			}
		}

		b.close();

		System.gc();

	}
}
