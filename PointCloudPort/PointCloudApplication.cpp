//
//  PointCloudApplication.cpp
//  PointCloudPort
//
//  Created by Michele Pratusevich on 2/7/13.
//  Copyright (c) 2013 Michele Pratusevich. All rights reserved.
//

#include "PointCloudApplication.h"
#include <cassert>


PointCloudApplication::PointCloudApplication(int viewport_width, int viewport_height,
											 int video_width, int video_height,
											 pointcloud_video_format video_format,
											 const char* _resource_path,
											 const char* documents_path,
											 const char* device,
											 double ui_scale_factor_) {
	ui_scale_factor = ui_scale_factor_;
	
	video_texture = 0;
	last_frame_clock = current_time();
	
	stop_opengl = false;
	
	pointcloud_create(viewport_width, viewport_height,
					  video_width, video_height,
					  video_format,
					  device,
					  "5fee8d84-72f5-4491-af82-e2a1e9bd0bab");
	
	context = pointcloud_get_context();
	
	resource_path = _resource_path;
    
	setup_graphics();
	
	init_lighting();
}


void PointCloudApplication::setup_graphics() {
	// Define Viewport
	glViewport(0,
			   0,
			   context.viewport_width * ui_scale_factor,
			   context.viewport_height * ui_scale_factor);
	
	setup_video_texture();
	
	logo_texture  = read_png_texture("pointcloud-logo.png");
	point_texture = read_png_texture("trackpoint.png");
	
	switch_to_ortho();
}


void PointCloudApplication::setup_video_texture() {
	assert(context.video_format == POINTCLOUD_BGRA_8888);
	
	video_texture = create_texture(NULL, context.video_width, context.video_height, true, GL_BGRA);
    
	// Setup geometry for the video overlay
	video_vertices[0] = 0;
	video_vertices[1] = context.viewport_height;
	video_vertices[2] = context.viewport_width;
	video_vertices[3] = context.viewport_height;
	video_vertices[4] = context.viewport_width;
	video_vertices[5] = 0;
	video_vertices[6] = 0;
	video_vertices[7] = 0;
	
	video_texcoords[0] = context.video_width  - context.video_crop_x;
	video_texcoords[1] = context.video_height - context.video_crop_y;
	video_texcoords[2] = context.video_width  - context.video_crop_x;
	video_texcoords[3] = context.video_crop_y;
	video_texcoords[4] = context.video_crop_x;
	video_texcoords[5] = context.video_crop_y;
	video_texcoords[6] = context.video_crop_x;
	video_texcoords[7] = context.video_height - context.video_crop_y;
    
	for (int i = 0; i < 8; i += 2) {
		video_texcoords[i] /= context.video_width;
		video_texcoords[i+1] /= context.video_height;
	}
}


void PointCloudApplication::load_camera_texture(char *data) {
	if (!data)
		return;
    
	glBindTexture(GL_TEXTURE_2D, video_texture);
    
	assert(context.video_format == POINTCLOUD_BGRA_8888);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
					context.video_width, context.video_height,
					GL_BGRA, GL_UNSIGNED_BYTE, data);
}


void PointCloudApplication::render_camera_frame() {
	switch_to_ortho();
	
	//glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(/*GL_COLOR_BUFFER_BIT | */GL_DEPTH_BUFFER_BIT);
	
	glBindTexture(GL_TEXTURE_2D, video_texture);
    
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glVertexPointer(2, GL_FLOAT, 0, video_vertices);
	glTexCoordPointer( 2, GL_FLOAT, 0, video_texcoords);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
}

void PointCloudApplication::clean_up() {
}

/*
 * Draws the required (for the free SDK) logo in the bottom-left corner of the visible camera feed
 */
void PointCloudApplication::draw_logo() {
	int h = context.viewport_height;
	int w = context.viewport_width;
	double scale_factor = w == 320 ? 0.5 : 1.0;
	switch_to_ortho();
	draw_image(logo_texture, 10, h-(140+54)*scale_factor, 262*scale_factor, 54*scale_factor, 0, 0, 1, 1);
}



void PointCloudApplication::on_pause() {
	// We need to stop all OpenGL calls as soon as the application enters background
	stop_opengl = true;
}


void PointCloudApplication::on_resume() {
	stop_opengl = false;
}


void PointCloudApplication::on_accelerometer_update(float x, float y, float z, double timestamp) {
	pointcloud_on_accelerometer_update(x, y, z, timestamp);
}


void PointCloudApplication::on_device_motion_update(float x, float y, float z, float rot_x, float rot_y, float rot_z, float g_x, float g_y, float g_z, double timestamp) {
	pointcloud_on_device_motion_update(x, y, z, rot_x, rot_y, rot_z, g_x, g_y, g_z, timestamp);
}



/**
 * Main rendering loop
 */
void PointCloudApplication::render_frame(char *data, int length, double timestamp) {
	if (stop_opengl) return;
	
	pointcloud_state state = pointcloud_get_state();
	
	// Timers
	double now = current_time();
	double time_since_last_frame = now - last_frame_clock;
	last_frame_clock = now;
	
	load_camera_texture(data);
	if (stop_opengl) return;
	
	process_camera_frame(data, timestamp);
	if (stop_opengl) return;
	
	render_camera_frame();
	if (stop_opengl) return;
	
	render_point_cloud();
	if (stop_opengl) return;
	
	render_content(time_since_last_frame);
	if (stop_opengl) return;
	
	draw_logo();
	if (stop_opengl) return;
	
	clean_up();
	
	last_state = state;
}


void PointCloudApplication::process_camera_frame(char *data, double timestamp) {
	
	pointcloud_on_camera_frame(data, timestamp);
	
	camera_matrix = pointcloud_get_camera_matrix();
	
	// Calculate the camera projection matrix (with given near and far clipping planes)
	projection_matrix = pointcloud_get_frustum(0.1, 100);
}

/*
 * Renders the tracked points
 */
void PointCloudApplication::render_point_cloud() {
	
    pointcloud_state state = pointcloud_get_state();
    
    if (state == POINTCLOUD_INITIALIZING ||
		state == POINTCLOUD_TRACKING_SLAM_MAP) {
        
        pointcloud_point_cloud* points = pointcloud_get_points();
		
        if (points) {
			switch_to_camera();
			disable_lighting();
            
			glDisable(GL_DEPTH_TEST);
            
            glColor4f(0.9, 0.95, 1.0, 0.6);
            
            glEnable(GL_POINT_SPRITE_OES);
            glEnable(GL_TEXTURE_2D);
            
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			glBindTexture(GL_TEXTURE_2D, point_texture);
            
            glTexEnvi(GL_POINT_SPRITE_OES, GL_COORD_REPLACE_OES, GL_TRUE);
            
            glPointParameterf(GL_POINT_SIZE_MAX, 64.0f);
            
			glPointSize(32.0);
			glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(3,GL_FLOAT,0, (float *)points->points);
            glDrawArrays(GL_POINTS,0, points->size);
			
			glDisableClientState(GL_VERTEX_ARRAY);
            
            glColor4f(1, 1, 1, 1);
			
			glPointSize(1);
            
			glDisable(GL_BLEND);
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_POINT_SPRITE_OES);
			
			pointcloud_destroy_point_cloud(points);
			
			switch_to_ortho();
        }
    }
}


/*
 * Switches to orthogonal drawing (e.g. UI)
 */
void PointCloudApplication::switch_to_ortho() {
	disable_lighting();
	
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0, context.viewport_width, context.viewport_height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

/*
 * Switches to camera perspective ("AR mode")
 */
void PointCloudApplication::switch_to_camera() {
	enable_lighting();
	
	glShadeModel(GL_SMOOTH);
	
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	
	glEnable(GL_DEPTH_TEST);
	
	// Set up projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection_matrix.data);
    
	// Set up camera matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(camera_matrix.data);
}


/*
 * Initializes basic lighting
 */
void PointCloudApplication::init_lighting() {
	float light_ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
	float light_diffuse[4] = {1.0f, 1.0f, 1.0, 1.0f};
	float light_specular[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	
	// Assign created components to GL_LIGHT0
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
}


void PointCloudApplication::enable_lighting() {
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}


void PointCloudApplication::disable_lighting() {
	glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);
}
