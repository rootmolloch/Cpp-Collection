#include <CL/cl.h>
#include <iostream>
#include <gtk/gtk.h>
#include <cstdlib>
#include <cmath>
#include <map>
#include <cstdlib>
#include <string>

using namespace std;

cl_context ctx;
cl_command_queue cqu;
cl_program prg;
cl_kernel kernel;
GtkWidget *main_window;
GtkWidget *video_window;
GtkWidget *fit_toggle_button;
GtkWidget *animate_toggle_button;
GtkWidget *animate2_toggle_button;
const int numCoeffs = 6;
GtkWidget *counterRSliders[numCoeffs];
GtkWidget *counterISliders[numCoeffs];
GtkWidget *denominatorRSliders[numCoeffs];
GtkWidget *denominatorISliders[numCoeffs];
char buffer[] = {numCoeffs + '0',0};
bool doRender = true;


struct MyConfig {
	int width;
	int height;
	float mathx0;
	float mathx1;
	float mathy0;
	float mathy1;
	float counterR[numCoeffs];
	float counterI[numCoeffs];
	float denominatorR[numCoeffs];
	float denominatorI[numCoeffs];
};

const char *source[] = {
	"struct MyConfig {",
	"	int width;",
	"	int height;",
	"	float mathx0;",
	"	float mathx1;",
	"	float mathy0;",
	"	float mathy1;",
	"	float counterR[",buffer,"];",
	"	float counterI[",buffer,"];",
	"	float denominatorR[",buffer,"];",
	"	float denominatorI[",buffer,"];",
	"};",
	"\n#define M_PI 3.141f\n",
	"__kernel void fractal(__global struct MyConfig *cfg,__global uchar *img,__global ushort *iterations,int shift) {",
	"	int id = get_global_id(0);"
	"	int x = id % cfg->width;",
	"	int y = id / cfg->width;",
	"	float mathx = (cfg->mathx1 - cfg->mathx0) * (float)x / (float)cfg->width + cfg->mathx0;",
	"	float mathy = (cfg->mathy1 - cfg->mathy0) * (float)y / (float)cfg->height + cfg->mathy0;",
	"	float xr = 0;",
	"	float xi = 0;",
	"	int k = 1;",
	"	while(xr * xr + xi * xi < 10.0) {",
	"		float zr,zi;",
	"		zr = 1.0;",
	"		zi = 0.0;",
	"		float counterPolyR = cfg->counterR[1] * mathx + cfg->counterR[0];",
	"		float counterPolyI = cfg->counterI[1] * mathy + cfg->counterI[0];",
	"		for(int i = 2;i < sizeof(cfg->counterR) / sizeof(float);++i) {",
	"			float zr2 = zr * xr - zi * xi;",
	"			zi = zr * xi + zi * xr;",
	"			zr = zr2;",
	"			counterPolyR += cfg->counterR[i] * zr - cfg->counterI[i] * zi;",
	"			counterPolyI += cfg->counterR[i] * zi + cfg->counterI[i] * zr;",
	"		}",
	"		zr = 1.0;",
	"		zi = 0.0;",
	"		float denominatorPolyR = cfg->denominatorR[1] * mathx + cfg->denominatorR[0];",
	"		float denominatorPolyI = cfg->denominatorI[1] * mathy + cfg->denominatorI[0];",
	"		for(int i = 2;i < sizeof(cfg->counterR) / sizeof(float);++i) {",
	"			float zr2 = zr * xr - zi * xi;",
	"			zi = zr * xi + zi * xr;",
	"			zr = zr2;",
	"			denominatorPolyR += cfg->denominatorR[i] * zr - cfg->denominatorI[i] * zi;",
	"			denominatorPolyI += cfg->denominatorR[i] * zi + cfg->denominatorI[i] * zr;",
	"		}",
	"		float z = denominatorPolyR * denominatorPolyR + denominatorPolyI * denominatorPolyI;",
	"		xr = (counterPolyR * denominatorPolyR + counterPolyI * denominatorPolyI) / z;",
	"		xi = (-counterPolyR * denominatorPolyI + counterPolyI * denominatorPolyR) / z;",
	"		k += 1;",
	"		if(k > 200) break;",
	"	}",
	"	uchar r,g,b;",
	"	if(k > 200) {",
	"		r = 0;",
	"		g = 0;",
	"		b = 0;",
	"		k = 0;",
	"	} else {",
	"		k += shift;",
	"		r = 250.0f * (sin(0.05f * (float)k * 2.0f * M_PI) + 1.0f) / 2.0f;",
	"		g = 250.0f * (sin(0.06f * (float)k * 2.0f * M_PI) + 1.0f) / 2.0f;",
	"		b = 250.0f * (sin(0.07f * (float)k * 2.0f * M_PI) + 1.0f) / 2.0f;",
	"	}",
	"	iterations[x + cfg->width * y] = k;",
	"	img[4 * (x + cfg->width * y)] = b;", // Blue
	"	img[4 * (x + cfg->width * y) + 1] = g;", // Green
	"	img[4 * (x + cfg->width * y) + 2] = r;", // Red
	"	img[4 * (x + cfg->width * y) + 3] = 0;",
	"}"
};

const int numSourceLines = sizeof(source) / sizeof(char *);
size_t sourceSZs[numSourceLines];


static void delete_event_cb (GtkWidget *widget, GdkEvent *event, void *data) {
	gtk_main_quit ();
}

cl_int myShift = 0;

static void compute(unsigned char *img,uint16_t *iterations,MyConfig &myConfig) {
	int err;
	
	size_t sz = myConfig.width * myConfig.height;

	cl_mem configBuffer = clCreateBuffer(ctx,CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,sizeof(myConfig),&myConfig,&err);
	if(err) cout << "allocated config buffer with err = " << err << endl;
	cl_mem imageBuffer = clCreateBuffer(ctx,CL_MEM_WRITE_ONLY,sz * 4,NULL,&err);
	if(err) cout << "allocated image buffer with err = " << err << endl;
	cl_mem iterationsBuffer = clCreateBuffer(ctx,CL_MEM_WRITE_ONLY,sz * 2,NULL,&err);
	if(err) cout << "allocated iterations buffer with err = " << err << endl;
	
	err = clSetKernelArg(kernel,0,sizeof(cl_mem),&configBuffer);
	if(err) cout << "set kernel arg 0 with err = " << err << endl;

	err = clSetKernelArg(kernel,1,sizeof(cl_mem),&imageBuffer);
	if(err) cout << "set kernel arg 1 with err = " << err << endl;

	err = clSetKernelArg(kernel,2,sizeof(cl_mem),&iterationsBuffer);
	if(err) cout << "set kernel arg 2 with err = " << err << endl;

	err = clSetKernelArg(kernel,3,sizeof(cl_int),&myShift);
	if(err) cout << "set kernel arg 3 with err = " << err << endl;

	err = clEnqueueNDRangeKernel(cqu,kernel,1,0,&sz,NULL,0,NULL,NULL);
	if(err) cout << "enqueued kernel with err = " << err << endl;

	if(img) err = clEnqueueReadBuffer(cqu,imageBuffer,CL_TRUE,0,sz * 4,img,0,NULL,NULL);
	if(iterations) err = clEnqueueReadBuffer(cqu,iterationsBuffer,CL_TRUE,0,sz * 2,iterations,0,NULL,NULL);

	err = clReleaseMemObject(iterationsBuffer);
	if(err) cout << "released iterations buffer with err = " << err << endl;
	err = clReleaseMemObject(imageBuffer);
	if(err) cout << "released image buffer with err = " << err << endl;
	err = clReleaseMemObject(configBuffer);
	if(err) cout << "released config buffer with err = " << err << endl;
}


MyConfig myConfig;

static gboolean draw_cb (GtkWidget *widget, cairo_t *cr, void *data) {
	GtkAllocation allocation;
	gtk_widget_get_allocation (widget, &allocation);
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_fill (cr);

	myConfig.width = allocation.width;
	myConfig.height = allocation.height;
	
	unsigned char img[allocation.width * allocation.height * 4];
	compute(img,NULL,myConfig);

	cairo_surface_t *srf = cairo_image_surface_create_for_data(img,CAIRO_FORMAT_RGB24,allocation.width,allocation.height,allocation.width * 4);
	cairo_set_source_surface (cr,srf,0,0);
	cairo_paint(cr);
	cairo_surface_destroy(srf);

	return true;
}

int mousex0 = 0;
int mousey0 = 0;

gboolean cb_home(GtkWidget *btn, GdkEventButton *event, void *ptr) {
	myConfig.mathx0 = -20;
	myConfig.mathx1 = 20;
	myConfig.mathy0 = -20;
	myConfig.mathy1 = 20;
	
	gtk_widget_queue_draw(video_window);
}

gboolean cb_press(GtkWidget *btn, GdkEventButton *event, void *ptr) {	
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 2)
	{
		mousex0 = event->x;
		mousey0 = event->y;
	}
}


float sqr(float x) {
	return pow(x,10);
}

float sqrroot(const float x) {
	return pow(x,0.1);
}


gboolean cb_rand(GtkWidget *btn, GdkEventButton *event, void *ptr) {	
	for(int i = 0;i < numCoeffs;++i) {
		myConfig.counterR[i] = sqr((double)rand() / RAND_MAX);
		myConfig.counterI[i] = sqr((double)rand() / RAND_MAX);
		myConfig.denominatorR[i] = sqr((double)rand() / RAND_MAX);
		myConfig.denominatorI[i] = sqr((double)rand() / RAND_MAX);
	}
	myConfig.counterR[numCoeffs - 1] = sqrroot((double)rand() / RAND_MAX);
	
	myConfig.counterR[0] = sqr((double)rand() / RAND_MAX);
	myConfig.counterI[0] = sqr((double)rand() / RAND_MAX);
	myConfig.counterR[1] = sqrroot((double)rand() / RAND_MAX);
	myConfig.counterI[1] = sqrroot((double)rand() / RAND_MAX);
	myConfig.counterR[numCoeffs - 1] = sqrroot((double)rand() / RAND_MAX);
	
	/*
	for(int i = 0;i < 2;++i) {
		if(rand() % 4 <= 2) myConfig.counterR[rand() % (numCoeffs - 2) + 2] = (double)rand() / RAND_MAX;
		if(rand() % 4 <= 2) myConfig.counterI[rand() % (numCoeffs - 2) + 2] = (double)rand() / RAND_MAX;
	}
	*/
	
	myConfig.denominatorR[0] = sqrroot((double)rand() / RAND_MAX);
	myConfig.denominatorI[0] = sqr((double)rand() / RAND_MAX);
	myConfig.denominatorR[1] = sqrroot((double)rand() / RAND_MAX);
	myConfig.denominatorI[1] = sqrroot((double)rand() / RAND_MAX);
	
	for(int i = 0;i < 1;++i) {
		if(rand() % 4 <= 2) myConfig.denominatorR[rand() % (numCoeffs - 2) + 2] = (double)rand() / RAND_MAX;
		if(rand() % 4 <= 2) myConfig.denominatorI[rand() % (numCoeffs - 2) + 2] = (double)rand() / RAND_MAX;
	}
	
	/*
	for(int i = 0;i < numCoeffs;++i) {
			myConfig.counterR[i] += (1.0 - (double)i / numCoeffs) * (double)rand() / RAND_MAX * (rand() % 2 ? -1 : 1);
			myConfig.counterI[i] += (1.0 - (double)i / numCoeffs) * (double)rand() / RAND_MAX * (rand() % 2 ? -1 : 1);
			myConfig.denominatorR[i] += (1.0 - (double)i / numCoeffs) * (double)rand() / RAND_MAX * (rand() % 2 ? -1 : 1);
			myConfig.denominatorI[i] += (1.0 - (double)i / numCoeffs) * (double)rand() / RAND_MAX * (rand() % 2 ? -1 : 1);
	}

	for(int i = 0;i < numCoeffs;++i) {
			if(rand() % 2) myConfig.counterR[i] *= -1;
			if(rand() % 2) myConfig.counterI[i] *= -1;
			if(rand() % 2) myConfig.denominatorR[i] *= -1;
			if(rand() % 2) myConfig.denominatorI[i] *= -1;
	}
*/

	doRender = false;
	for(int i = 0;i < numCoeffs;++i) {
		gtk_range_set_value (GTK_RANGE (counterISliders[i]), myConfig.counterI[i]);
		gtk_range_set_value (GTK_RANGE (counterRSliders[i]), myConfig.counterR[i]);
		gtk_range_set_value (GTK_RANGE (denominatorISliders[i]), myConfig.denominatorI[i]);
		gtk_range_set_value (GTK_RANGE (denominatorRSliders[i]), myConfig.denominatorR[i]);
	}
	doRender = true;
		
	gtk_widget_queue_draw(video_window);
}

gboolean cb_motion(GtkWidget *btn, GdkEventMotion *event, void *ptr) {
	if (event->state & GDK_BUTTON2_MASK) {
		float dx = (event->x - mousex0) * (myConfig.mathx1 - myConfig.mathx0) / myConfig.width;
		float dy = (event->y - mousey0) * (myConfig.mathy1 - myConfig.mathy0) / myConfig.height;
		
		myConfig.mathx0 -= dx;
		myConfig.mathx1 -= dx;
		myConfig.mathy0 -= dy;
		myConfig.mathy1 -= dy;
		mousex0 = event->x;
		mousey0 = event->y;
		
		gtk_widget_queue_draw(video_window);
	}
}

gboolean cb_scroll(GtkWidget *btn, GdkEventScroll *event, void *ptr) {
	if(event->direction == GDK_SCROLL_UP) {
		myConfig.mathx0 += 0.1 * (myConfig.mathx1 - myConfig.mathx0);
		myConfig.mathx1 -= 0.1 * (myConfig.mathx1 - myConfig.mathx0);
		myConfig.mathy0 += 0.1 * (myConfig.mathy1 - myConfig.mathy0);
		myConfig.mathy1 -= 0.1 * (myConfig.mathy1 - myConfig.mathy0);
		
		gtk_widget_queue_draw(video_window);
		
	} 

	if(event->direction == GDK_SCROLL_DOWN) {
		myConfig.mathx0 -= 0.1 * (myConfig.mathx1 - myConfig.mathx0);
		myConfig.mathx1 += 0.1 * (myConfig.mathx1 - myConfig.mathx0);
		myConfig.mathy0 -= 0.1 * (myConfig.mathy1 - myConfig.mathy0);
		myConfig.mathy1 += 0.1 * (myConfig.mathy1 - myConfig.mathy0);
		
		gtk_widget_queue_draw(video_window);
		
	} 
}

static void counterRSlider_cb (GtkRange *range, void *data) {
	int i = (long)data;
	myConfig.counterR[i] = gtk_range_get_value (GTK_RANGE (counterRSliders[i]));
	if(doRender) gtk_widget_queue_draw(video_window);
}

static void counterISlider_cb (GtkRange *range, void *data) {
	int i = (long)data;
	myConfig.counterI[i] = gtk_range_get_value (GTK_RANGE (counterISliders[i]));
	if(doRender) gtk_widget_queue_draw(video_window);
}

static void denominatorRSlider_cb (GtkRange *range, void *data) {
	int i = (long)data;
	myConfig.denominatorR[i] = gtk_range_get_value (GTK_RANGE (denominatorRSliders[i]));
	if(doRender) gtk_widget_queue_draw(video_window);
}

static void denominatorISlider_cb (GtkRange *range, void *data) {
	int i = (long)data;
	myConfig.denominatorI[i] = gtk_range_get_value (GTK_RANGE (denominatorISliders[i]));
	if(doRender) gtk_widget_queue_draw(video_window);
}

struct MyStats {
	float numColors;
	float colorVariance;
	float numBlackPixels;
	float average;
	
	void from(uint32_t *img,MyConfig &cfg) {
		map<uint32_t,uint16_t> m;
		
		for(int i = 0;i < cfg.width * cfg.height;++i) m[img[i]] = m[img[i]] + 1;
		
		numColors = m.size();
		
		average = 0;
		for(auto x : m) average += x.second;
		average /= m.size();

		colorVariance = 0;
		for(auto x : m) colorVariance += abs(x.second - colorVariance);
		colorVariance /= m.size();
		
		numBlackPixels = 0;
		for(int i = 0;i < cfg.width * cfg.height;++i) if(!img[i]) numBlackPixels += 1;
	}
};


static float xs[numCoeffs][4];
static float ys[numCoeffs][4];

inline float f(const float x) {
	return (x > 0 ? 1 : -1) * pow(abs(x),0.8);
}

gboolean cb_animate2(gpointer user_data) {
	static float x = 0;

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(animate2_toggle_button))) {
		x += 0.1;
		MyConfig cfg = myConfig;
	
		for(int i = 0;i < numCoeffs;++i) {
			cfg.counterR[i] += 0.01 * (0.5 * rand() / RAND_MAX + 0.5) * (sin(0.1 * xs[i][0] * x + ys[i][0]));
			cfg.counterI[i] += 0.01 * (0.5 * rand() / RAND_MAX + 0.5) * (sin(0.1 * xs[i][1] * x + ys[i][1]));
			cfg.denominatorR[i] += 0.001 * (0.5 * rand() / RAND_MAX + 0.5) * (sin(0.1 * xs[i][2] * x + ys[i][2]));
			cfg.denominatorI[i] += 0.001 * (0.5 * rand() / RAND_MAX + 0.5) * (sin(0.1 * xs[i][3] * x + ys[i][3]));
		}

		myConfig = cfg;
		
		doRender = false;
		for(int i = 0;i < numCoeffs;++i) {
			gtk_range_set_value (GTK_RANGE (counterISliders[i]), myConfig.counterI[i]);
			gtk_range_set_value (GTK_RANGE (counterRSliders[i]), myConfig.counterR[i]);
			gtk_range_set_value (GTK_RANGE (denominatorISliders[i]), myConfig.denominatorI[i]);
			gtk_range_set_value (GTK_RANGE (denominatorRSliders[i]), myConfig.denominatorR[i]);
		}
		doRender = true;

		gtk_widget_queue_draw(video_window);
	}
	
	g_timeout_add(15,cb_animate2,NULL);
	return false;
}

gboolean cb_animate(gpointer user_data) {
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(animate_toggle_button))) {
		MyConfig cfg0 = myConfig;
		MyConfig cfg = cfg0;
		
		uint32_t img[cfg0.width * cfg0.height];
		compute((unsigned char *)img,NULL,cfg0);
		
		MyConfig best[50];
		MyStats stats[50];
		
		for(int h = 0;h < 3;++h) {
			MyConfig cfg = cfg0;
			MyConfig delta;
			
			float sum = 0;
			for(int i = 0;i < numCoeffs;++i) {
				delta.counterR[i] = (float)rand() / RAND_MAX - 0.5;
				delta.counterI[i] = (float)rand() / RAND_MAX - 0.5;
				delta.denominatorR[i] = (float)rand() / RAND_MAX - 0.5;
				delta.denominatorI[i] = (float)rand() / RAND_MAX - 0.5;
				
				sum += delta.counterR[i] * delta.counterR[i];
				sum += delta.counterI[i] * delta.counterI[i];
				sum += delta.denominatorR[i] * delta.denominatorR[i];
				sum += delta.denominatorI[i] * delta.denominatorI[i];
			}
			sum = sqrt(sum);

			float step = 0.05 * rand() / RAND_MAX + 0.05;
			for(int i = 0;i < numCoeffs;++i) {
				delta.counterR[i] *= step / sum;
				delta.counterI[i] *= step / sum;
				delta.denominatorR[i] *= step / sum;
				delta.denominatorI[i] *= step / sum;
			}
			
			for(int i = 0;i < numCoeffs;++i) {
				cfg.counterR[i] += delta.counterR[i];
				cfg.counterI[i] += delta.counterI[i];
				cfg.denominatorR[i] += delta.denominatorR[i];
				cfg.denominatorI[i] += delta.denominatorI[i];
			}

			compute((unsigned char *)img,NULL,cfg);
			best[h] = cfg;
			stats[h].from(img,cfg);
		}

		float sz = cfg.width * cfg.height * 0.5;
		int k = 0;
		int j = 0;
		int l = 0;
		for(int i = 1;i < 3;++i) {
			if(abs(stats[i].numBlackPixels - sz) < abs(stats[k].numBlackPixels - sz)) k = i;
			if(stats[i].numColors > stats[j].numColors) j = i;
			if(stats[i].colorVariance < stats[l].colorVariance) l = i;
		}
		
		switch(rand() % 3) {
			case 0: cfg = best[k];break;
			case 1: cfg = best[j];break;
			case 2: cfg = best[l];break;
		}

		myConfig = cfg;

		doRender = false;
		for(int i = 0;i < numCoeffs;++i) {
			gtk_range_set_value (GTK_RANGE (counterISliders[i]), myConfig.counterI[i]);
			gtk_range_set_value (GTK_RANGE (counterRSliders[i]), myConfig.counterR[i]);
			gtk_range_set_value (GTK_RANGE (denominatorISliders[i]), myConfig.denominatorI[i]);
			gtk_range_set_value (GTK_RANGE (denominatorRSliders[i]), myConfig.denominatorR[i]);
		}
		doRender = true;

		if(
			!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(animate2_toggle_button))
			&& !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(animate_toggle_button))
		)
			gtk_widget_queue_draw(video_window);
	}
	
	g_timeout_add(10,cb_animate,NULL);
	return false;
}

gboolean cb_shift(gpointer user_data) {
	myShift += 1;

	if(
		!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(animate2_toggle_button))
		&& !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(animate_toggle_button))
	)
		gtk_widget_queue_draw(video_window);
	
	g_timeout_add(100,cb_shift,NULL);
	return false;
}
	
gboolean cb_fit(gpointer user_data) {
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(fit_toggle_button))) {
		MyConfig cfg0 = myConfig;
		MyConfig cfg = cfg0;

		uint16_t iterations[cfg0.width * cfg0.height];
		compute(NULL,iterations,cfg);
		
		int minx = 10000;
		int maxx = -1;
		int miny = 10000;
		int maxy = -1;
		
		for(int y = 0;y < cfg.height;++y) {
			for(int x = 0;x < cfg.width;++x) {
				if(iterations[x + y * cfg.width] > 2) 
					{
						if(x < minx) minx = x;
						if(x > maxx) maxx = x;
						if(y < miny) miny = y;
						if(y > maxy) maxy = y;
					}
			}
		}
		
		cfg.mathx0 = (minx - 40) * (cfg.mathx1 - cfg.mathx0) / (cfg.width - 1) + cfg.mathx0;
		cfg.mathx1 = (maxx + 40) * (cfg.mathx1 - cfg.mathx0) / (cfg.width - 1) + cfg.mathx0;
		cfg.mathy0 = (miny - 40) * (cfg.mathy1 - cfg.mathy0) / (cfg.height - 1) + cfg.mathy0;
		cfg.mathy1 = (maxy + 40) * (cfg.mathy1 - cfg.mathy0) / (cfg.height - 1) + cfg.mathy0;
		
		myConfig = cfg;
		gtk_widget_queue_draw(video_window);
	}
		
	g_timeout_add(2,cb_fit,NULL);
	return false;
}

int main(int argc,char **argv) {
	srand(time(0));

	myConfig.mathx0 = -10;
	myConfig.mathx1 = 10;
	myConfig.mathy0 = -10;
	myConfig.mathy1 = 10;
	
	myConfig.counterR[1] = 1.0;
	myConfig.counterI[1] = 1.0;
	myConfig.denominatorR[0] = 1.0;
	myConfig.denominatorR[1] = 1;
	myConfig.denominatorI[1] = 1;
	myConfig.counterR[numCoeffs - 1] = 1.0;

	cl_platform_id platforms[10];
	cl_char platformInfos[10][100];
	cl_device_id deviceIDs[10][10];
	cl_char deviceNames[10][10][100];
	cl_uint numPlatforms;
	cl_uint numDevices[10];

	clGetPlatformIDs(10,platforms,&numPlatforms);
	cout << "found " << numPlatforms << " platforms" << endl;

	for(int i = 0;i < numCoeffs;++i) {
		for(int j = 0;j < 4;++j) {
			xs[i][j] = (double)rand() / RAND_MAX;
			ys[i][j] = 10.0 * rand() / RAND_MAX;
		}
	}

	for(int i = 0;i < numPlatforms;++i) {
		size_t sz;
		clGetPlatformInfo(platforms[i],CL_PLATFORM_NAME,100,platformInfos[i],&sz);
		platformInfos[i][sz] = 0;
		cout << i << ". " << platformInfos[i] << " ";
		
		clGetDeviceIDs(platforms[i],CL_DEVICE_TYPE_ALL,10,deviceIDs[i],&numDevices[i]);
		cout << "(#" << numDevices[i] << " devices : ";
		
		for(int j = 0;j < numDevices[i];++j) {
			clGetDeviceInfo(deviceIDs[i][j],CL_DEVICE_NAME,100,deviceNames[i][j],&sz);
			deviceNames[i][j][sz] = 0;
		}

		cout << deviceNames[i][0];
		for(int j = 1;j < numDevices[i];++j) {
			cout << ", " << deviceNames[i][0];
		}
		
		cout << ")" << endl;
	}
	
	cout << "which platform do you want to use ? ";
	int pi = 0;
	if(numPlatforms > 1) 
		cin >> pi;
	else
		cout << endl;

	for(int j = 0;j < numDevices[pi];++j) {
		cout << j << ". " << deviceNames[pi][0] << endl;
	}

	cout << "which device do you want to use ? ";
	int pj = 0;
	if(numDevices[pi] > 1) 
		cin >> pj;
	else
		cout << endl;
 	
 	cl_context_properties props[] = {
 		CL_CONTEXT_PLATFORM,(cl_context_properties)platforms[pi],0
 	};
 	
	cl_int err;
	ctx = clCreateContext(props,1,&deviceIDs[pi][pj],NULL,NULL,&err);
	if(err) {
		cout << "allocated context with err = " << err << endl;
		return -1;
	}
	
	cqu = clCreateCommandQueueWithProperties(ctx,deviceIDs[pi][pj],NULL,&err);
	if(err) cout << "allocated command queue with err = " << err << endl;

	for(int i = 0;i < numSourceLines;++i) {
		sourceSZs[i] = strlen(source[i]);
	}
	
	prg = clCreateProgramWithSource(ctx,numSourceLines,source,sourceSZs,&err);
	if(err) cout << "allocated program with err = " << err << endl;

	err = clBuildProgram(prg,1,&deviceIDs[pi][pj],"",NULL,NULL);
	if(err) cout << "built program with err = " << err << endl;

	size_t sz;
	char param[10000];
	clGetProgramBuildInfo(prg,deviceIDs[pi][pj],CL_PROGRAM_BUILD_LOG,10000,param,&sz);
	param[sz] = 0;
	if(err) cout << "program build log = " << err << endl << "sz : " << sz << endl << param << endl;

	kernel = clCreateKernel(prg,"fractal",&err);
	if(err) cout << "allocated kernel with err = " << err << endl;
	
	gtk_init (&argc, &argv);
	main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT (main_window), "delete-event", G_CALLBACK (delete_event_cb), NULL);
	video_window = gtk_drawing_area_new ();
	g_signal_connect (video_window, "draw", G_CALLBACK (draw_cb), NULL);
	g_signal_connect (video_window, "button-press-event",G_CALLBACK (cb_press), NULL);
	g_signal_connect (video_window, "motion-notify-event",G_CALLBACK (cb_motion), NULL);
	g_signal_connect (video_window, "scroll-event",G_CALLBACK (cb_scroll), NULL);
	gtk_widget_set_events (video_window,GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	
	GtkWidget *counterBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	for(int i = 0;i < numCoeffs;++i) {
		counterRSliders[i] = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, -10.0, 10.0, 0.01);
		gtk_box_pack_start (GTK_BOX (counterBox), counterRSliders[i], FALSE, FALSE, 2);
		gtk_widget_set_size_request(counterRSliders[i],100,10);
		gtk_range_set_value (GTK_RANGE (counterRSliders[i]), myConfig.counterR[i]);
		g_signal_connect (G_OBJECT (counterRSliders[i]), "value-changed", G_CALLBACK (counterRSlider_cb), (void *)i);
		counterISliders[i] = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, -10.0, 10.0, 0.01);
		gtk_box_pack_start (GTK_BOX (counterBox), counterISliders[i], FALSE, FALSE, 2);
		gtk_widget_set_size_request(counterISliders[i],100,10);
		gtk_range_set_value (GTK_RANGE (counterISliders[i]), myConfig.counterI[i]);
		g_signal_connect (G_OBJECT (counterISliders[i]), "value-changed", G_CALLBACK (counterISlider_cb), (void *)i);
	}

	GtkWidget *denominatorBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	for(int i = 0;i < numCoeffs;++i) {
		denominatorRSliders[i] = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, -10.0, 10.0, 0.01);
		gtk_box_pack_start (GTK_BOX (denominatorBox), denominatorRSliders[i], FALSE, FALSE, 2);
		gtk_widget_set_size_request(denominatorRSliders[i],100,10);
		gtk_range_set_value (GTK_RANGE (denominatorRSliders[i]), myConfig.denominatorR[i]);
		g_signal_connect (G_OBJECT (denominatorRSliders[i]), "value-changed", G_CALLBACK (denominatorRSlider_cb), (void *)i);
		denominatorISliders[i] = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, -10.0, 10.0, 0.01);
		gtk_box_pack_start (GTK_BOX (denominatorBox), denominatorISliders[i], FALSE, FALSE, 2);
		gtk_widget_set_size_request(denominatorISliders[i],100,10);
		gtk_range_set_value (GTK_RANGE (denominatorISliders[i]), myConfig.denominatorI[i]);
		g_signal_connect (G_OBJECT (denominatorISliders[i]), "value-changed", G_CALLBACK (denominatorISlider_cb), (void *)i);
	}

	GtkWidget *buttonBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);	
	animate_toggle_button = gtk_toggle_button_new_with_label("Animate");
	animate2_toggle_button = gtk_toggle_button_new_with_label("Animate 2");
	fit_toggle_button = gtk_toggle_button_new_with_label("Fit");
	GtkWidget *home_button = gtk_button_new_with_label("Home");
	g_signal_connect (home_button, "button-press-event",G_CALLBACK (cb_home), NULL);
	GtkWidget *rand_button = gtk_button_new_with_label("Randomize");
	g_signal_connect (rand_button, "button-press-event",G_CALLBACK (cb_rand), NULL);

	gtk_box_pack_start (GTK_BOX (buttonBox), fit_toggle_button, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (buttonBox), animate_toggle_button, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (buttonBox), animate2_toggle_button, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (buttonBox), home_button, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (buttonBox), rand_button, TRUE, TRUE, 2);
	
	GtkWidget *mainBox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (mainBox2), video_window, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (mainBox2), buttonBox, FALSE, FALSE, 2);

	GtkWidget *mainBox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);	
	gtk_box_pack_start (GTK_BOX (mainBox), counterBox, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX (mainBox), mainBox2, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (mainBox), denominatorBox, FALSE, FALSE, 2);
	
	gtk_container_add (GTK_CONTAINER (main_window), mainBox);
	gtk_window_set_default_size (GTK_WINDOW (main_window), 600, 480);
	gtk_widget_show_all (main_window);
	
	
	//g_timeout_add(100,cb_fit,NULL);
	//g_timeout_add(100,cb_animate,NULL);
	//g_timeout_add(100,cb_animate2,NULL);
	//g_timeout_add(100,cb_shift,NULL);
	
	//cb_rand(NULL,NULL,NULL);
	//cb_home(NULL,NULL,NULL);
	//gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fit_toggle_button),true);
	//gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(animate2_toggle_button),true);
	
	gtk_main ();
	
	return 0;
}

