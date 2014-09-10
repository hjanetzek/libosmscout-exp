import static java.lang.System.out;

import org.osmscout.AbstractRoutingProfile;
import org.osmscout.Database;
import org.osmscout.DatabaseParameter;
import org.osmscout.DatabaseRef;
import org.osmscout.FastestPathRoutingProfile;
import org.osmscout.GeoCoord;
import org.osmscout.GeoCoords;
import org.osmscout.ObjectFileRef;
import org.osmscout.RefType;
import org.osmscout.RouteData;
import org.osmscout.RouteDescription;
import org.osmscout.RouteDescription.CrossingWaysDescription;
import org.osmscout.RouteDescription.DirectionDescription;
import org.osmscout.RouteDescription.NameDescription;
import org.osmscout.RouteDescription.StartDescription;
import org.osmscout.RouteDescription.TargetDescription;
import org.osmscout.RoutePostprocessor;
import org.osmscout.RoutePostprocessorList;
import org.osmscout.RouterParameter;
import org.osmscout.RoutingService;
import org.osmscout.TypeConfigRef;
import org.osmscout.Vehicle;
import org.osmscout.Way;

public class RoutingTest {
	public static void main(String[] args) {
		System.loadLibrary("osmscout_jni");

		DatabaseParameter p = new DatabaseParameter();
		Database b = new Database(p);
		b.addReference();

		// double startLat = 53.1;
		// double startLon = 8.8;
		// double targetLat = 53.1;
		// double targetLon = 8.81;

		double startLat = 53.081023;
		double startLon = 8.827904;
		double targetLat = 53.082521;
		double targetLon = 8.836981;
		String path = "/home/jeff/dev/libosmscout/maps/bremen-latest";

		if (args.length == 5) {
			path = args[0];
			startLat = Double.parseDouble(args[1]);
			startLon = Double.parseDouble(args[2]);
			targetLat = Double.parseDouble(args[3]);
			targetLon = Double.parseDouble(args[4]);
		} else {
			out.println("use: <mapdirectory> <startLat> <startLon> <targetLat> <targetLon>");
		}

		boolean ok = b.open(path);
		out.println("ok: " + ok);
		if (!ok)
			System.exit(1);

		// Vehicle vehicle = Vehicle.vehicleBicycle;
		Vehicle vehicle = Vehicle.vehicleFoot;

		RouterParameter routeParam = new RouterParameter();
		RoutingService router = new RoutingService(new DatabaseRef(b), routeParam, vehicle);
		router.addReference();

		router.open();

		if (!router.isOpen()) {
			out.println("cannot open routing database");
			System.exit(1);
		}

		TypeConfigRef typeConfig = b.getTypeConfig();
		RouteData data = new RouteData();
		RouteDescription description = new RouteDescription();

		AbstractRoutingProfile routingProfile = new FastestPathRoutingProfile();
		// AbstractRoutingProfile routingProfile = new
		// ShortestPathRoutingProfile();

		routingProfile.parametrizeForFoot(typeConfig.get(), 5);
		// routingProfile.parametrizeForBicycle(typeConfig.get(), 5);

		ObjectFileRef startObject = new ObjectFileRef();
		int[] startNodeIndex = { 0 };

		if (!router.getClosestRoutableNode(
				startLat, startLon, vehicle, 1000, startObject, startNodeIndex)) {
			out.println("Error while searching for routing node near start location!");
			System.exit(1);
		}

		if (startObject.invalid() || startObject.getType() == RefType.refNode) {
			out.print("Cannot find start node for start location!");
		}
		out.println("start node: " + startNodeIndex[0]);

		ObjectFileRef targetObject = new ObjectFileRef();

		int[] targetNodeIndex = { 0 };

		if (!router.getClosestRoutableNode(
				targetLat, targetLon, vehicle, 1000, targetObject, targetNodeIndex)) {
			out.println("Error while searching for routing node near target location!");
			System.exit(1);
		}

		if (targetObject.invalid() || targetObject.getType() == RefType.refNode) {
			out.print("Cannot find target node for target location!");
		}
		out.println("target node: " + targetNodeIndex[0]);

		if (!router.calculateRoute(
				routingProfile, startObject, startNodeIndex[0], targetObject, targetNodeIndex[0], data)) {
			out.println("There was an error while calculating the route!");
			router.close();
			System.exit(0);
		}

		if (data.isEmpty()) {
			out.println("No Route found!");
			router.close();
			System.exit(0);
		}

		if (!router.transformRouteDataToRouteDescription(data, description)) {
			out.println("Error during route transform");
		}

		RoutePostprocessorList post = new RoutePostprocessorList();
		post.add(RoutePostprocessor.createRouteDistanceAndTimeProcessor());
		post.add(RoutePostprocessor.createRouteStartProcessor("Here"));
		post.add(RoutePostprocessor.createRouteTargetProcessor("There"));
		post.add(RoutePostprocessor.createRouteWayNameProcessor());
		post.add(RoutePostprocessor.createRouteCrossingWaysProcessor());
		post.add(RoutePostprocessor.createRouteDirectionProcessor());

		RoutePostprocessor postprocessor = new RoutePostprocessor();
		if (!postprocessor.postprocessRouteDescription(description, routingProfile, b, post)) {
			out.println("Error during postprocessing");
		}

		for (RouteDescription.Node n : description.nodes()) {
			out.println("node > " + n.getDistance() + " " + n.getTime() + " "
					+ n.getCurrentNodeIndex() + " " + n.getTargetNodeIndex() + " "
					+ n.hasPathObject());

			NameDescription nameDesc = n.getNameDescription();
			if (nameDesc != null)
				out.println("name: " + nameDesc.getName() + " " + nameDesc.getRef() + " "
						+ nameDesc.getDescription());

			StartDescription startDesc = n.getStartDescription();
			if (startDesc != null)
				out.println("start: " + startDesc.getDescription());

			TargetDescription targetDesc = n.getTargetDescription();
			if (targetDesc != null)
				out.println("Target: " + targetDesc.getDescription());

			CrossingWaysDescription crossDesc = n.getCrossingWaysDescription();
			if (crossDesc != null)
				out.println("crossing: " + crossDesc.getExitCount() + " "
						+ crossDesc.getDebugString());

			DirectionDescription dirDesc = n.getDirectionDescription();
			if (dirDesc != null)
				out.println("direction: " + dirDesc.getDebugString());
		}

		Way way = new Way();
		router.transformRouteDataToWay(data, way);
		// int num = way.getNumNodes();
		// double[] nodes = new double[num * 2];
		// way.copyNodes(nodes);

		GeoCoords coords = way.getNodes();
		for (int i = 0, n = coords.size(); i < n; i++) {
			GeoCoord c = coords.get(i);
			out.println(c.getLat() + " " + c.getLon());
			// + " " + nodes[i * 2] + " " + nodes[i * 2 + 1]
		}

		b.close();
	}
}
