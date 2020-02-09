#include <gst/gst.h>
#include <vector>
#include <string>
#include <dirent.h>
#include <iostream>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <chrono>
#include <unistd.h>

using namespace std;

vector<string> myFiles;




int
main (int argc, char *argv[])
{
	if(argc != 2) {
		cerr << "usage : player <uri>" << endl;
		return EXIT_FAILURE;
	}

	GstElement *pipeline;
	GstBus *bus;
	GstMessage *msg;

	/* Initialize GStreamer */
	gst_init (&argc, &argv);

	int cnt = 0;

	while(1) {
		cout << cnt++ << endl;
		pipeline = gst_parse_launch ((string("playbin uri=") + argv[1]).c_str(), NULL);

		/* Start playing */
		gst_element_set_state (pipeline, GST_STATE_PLAYING);

		/* Wait until error or EOS */
		bus = gst_element_get_bus (pipeline);
		msg =
		gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
		(GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

		/* Free resources */
		if (msg != NULL)
		gst_message_unref (msg);
		gst_object_unref (bus);
		gst_element_set_state (pipeline, GST_STATE_NULL);
		gst_object_unref (pipeline);

		sleep(2);
	}
	return 0;
}

