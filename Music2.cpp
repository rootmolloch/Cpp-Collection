#include <gst/gst.h>
#include <glib.h>
#include <iostream>


using namespace std;



int main(int argc, char **argv) {
	GstElement *audioSink;
	
	gst_init (&argc, &argv);
	
	audioSink = gst_element_factory_make ("autoaudiosink", "audio-output");

	cout << audioSink << endl;

	return 0;
}








