#include <gst/gst.h>
#include <vector>
#include <string>
#include <dirent.h>
#include <iostream>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <chrono>


using namespace std;

vector<string> myFiles;




int
main (int argc, char *argv[])
{
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (argv[argc - 1])) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			myFiles.push_back(ent->d_name);
		}
		closedir (dir);
	} else {
		cerr << "could not list dir" << endl;
		return EXIT_FAILURE;
	}

	cout << "Found " << myFiles.size() << " entries" << endl;

	GstElement *pipeline;
	GstBus *bus;
	GstMessage *msg;

	/* Initialize GStreamer */

	gst_init (&argc,&argv);

	unsigned seed = chrono::system_clock::now().time_since_epoch().count();
	default_random_engine rng(seed);


	while(1) {
		shuffle(myFiles.begin(),myFiles.end(),rng);
	cout << myFiles.size() << endl;

		for(const auto fn : myFiles) {
			/* Build the pipeline */
			char full[PATH_MAX+1];
			string x = argv[argc - 1];
			x = x + "/" + fn;

			cout << realpath(x.c_str(),full) << endl;

			string ppl = string("playbin uri=file://") + full + string(" volume=") + argv[argc - 2];
			cout << ppl << endl;

			pipeline = gst_parse_launch (ppl.c_str(), NULL);

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
		}
	}
	return 0;
}

