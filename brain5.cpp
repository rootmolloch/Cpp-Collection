#include <iostream>
#include <gtk/gtk.h>
#include <cstdlib>
#include <cmath>
#include <map>
#include <cstdlib>
#include <string>
#include <limits>
#include <vector>
#include "myBib.hpp"


using namespace std;



GtkWidget *main_window;
GtkWidget *video_window;
GtkWidget *training_toggle_button;



gboolean draw_cb(GtkWidget *widget, cairo_t *cr, Physics2D *data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation (widget, &allocation);
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_fill (cr);
	
	cairo_set_line_width(cr, 5);
	cairo_set_source_rgb (cr, 0, 1, 0);
	cairo_new_path(cr);
	cairo_move_to(cr,0,0.5 * allocation.height);
	cairo_line_to(cr,allocation.width,0.5 * allocation.height);
	cairo_stroke(cr);
	
	cairo_set_source_rgb (cr, 1, 1, 1);
	for(auto l : data->lines) {
		cairo_new_path(cr);
		cairo_move_to(cr,allocation.width * l->p1->x,allocation.height * l->p1->y);
		cairo_line_to(cr,allocation.width * l->p2->x,allocation.height * l->p2->y);
		cairo_stroke(cr);
	}
	
	for(auto p : data->points) {
		cairo_new_path(cr);
		cairo_arc(cr,allocation.width * p->x,allocation.height * p->y,10,0,2 * M_PI);
		cairo_stroke(cr);
	}
}



typedef NN<3,5,1> MyNN;
Algorithm<MyNN,200> algorithm(2000);
Physics2D ps;



int cb_animate(void *) {
	static int step = 0;
	static int index = 0;
	int num;
	
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(training_toggle_button))) {
		num = 100000;
	} else {
		num = 1;
	}
	
	for(int k = 0;k < num;++k) {
		double inputs[] = {
			ps.agent->x,
			ps.pendulum->x,
			ps.pendulum->y
		};
		double output[1];

		algorithm.current().compute(inputs,output);
		ps.agent->vx += 0.1 * output[0];
		ps.agent->vx += 0.01 * (rand() % 2 ? 1 : -1);
		ps.step();
		
		if(algorithm.putReward(ps.pendulum->y)) {
			MyNN x = algorithm.linearCombination();
			algorithm.mutate(x);
		}
	}
	
	g_timeout_add(10,cb_animate,NULL);
	return false;
}



gboolean cb_render(void *x) {
	gtk_widget_queue_draw(video_window);
	return true;
}



static void delete_event_cb (GtkWidget *widget, GdkEvent *event, void *data) {
	gtk_main_quit ();
}



int main(int argc,char **argv) {
	srand(time(0));
	
	MyNN *nn = new MyNN();

	double inputs[] = {1,2,3};
	double output[1];
	nn->compute(inputs,output);
	cout << output[0] << endl;
	nn->compute(inputs,output);
	cout << output[0] << endl;
		
	gtk_init (&argc, &argv);
	main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (delete_event_cb), NULL);
	video_window = gtk_drawing_area_new ();
	g_signal_connect (video_window, "draw", G_CALLBACK (draw_cb), &ps);
 
	training_toggle_button = gtk_toggle_button_new_with_label("Training");
	
	GtkWidget *b1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (b1), training_toggle_button, TRUE, TRUE, 2);

	GtkWidget *b2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (b2), video_window, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (b2), b1, FALSE, FALSE, 2);
	
	gtk_container_add (GTK_CONTAINER (main_window), b2);
	gtk_window_set_default_size (GTK_WINDOW (main_window), 640, 480);
	gtk_widget_show_all (main_window);
	
	g_timeout_add(10,cb_animate,&nn);
	g_timeout_add(3,cb_render,NULL);

	gtk_main ();
	
	return 0;
}


