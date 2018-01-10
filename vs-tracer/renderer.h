#pragma once

#include <ctime>

#include "camera.h"
#include "hitable_list.h"

struct cu_hit {
	int hit_idx;
	float hit_t;
};

struct pixel {
	//TODO pixel should know it's coordinates and it's id should be a computed field
	uint id;
	uint samples;
	uint done; // needed to differentiate between done vs ongoing samples, when doing progressive rendering

	pixel(): id(0), samples(0), done(0) {}
};

struct work_unit {
	const uint start_idx;
	const uint end_idx;
	ray* rays;
	bool compact;
	bool done;

	work_unit(uint start, uint end, ray* _rays) :start_idx(start), end_idx(end), rays(_rays), compact(false), done(false) {}
	uint length() const { return end_idx - start_idx; }
};

class renderer {
public:
	renderer(camera* _cam, hitable_list* w, sphere *ls, unsigned int _nx, unsigned int _ny, unsigned int _ns, unsigned int _max_depth, float _min_attenuation) { 
		cam = _cam;
		world = w;
		nx = _nx; 
		ny = _ny; 
		ns = _ns;
		max_depth = _max_depth;
		min_attenuation = _min_attenuation;
		light_shape = ls;
	}

	unsigned int numpixels() const { return nx*ny; }
	bool is_not_done() const { return !(wunits[0]->done && wunits[1]->done); }
	unsigned int get_pixelId(int x, int y) const { return (ny - y - 1)*nx + x; }
	float3 get_pixel_color(int x, int y) const {
		const unsigned int pixelId = get_pixelId(x, y);
		if (pixels[pixelId].done == 0) return make_float3(0, 0, 0);
		return h_colors[pixelId] / float(pixels[pixelId].done);
	}
	uint totalrays() const { return total_rays; }

	void prepare_kernel();
	void update_camera();

	bool color(int ray_idx);
	void generate_rays();
	void render();

	void destroy();

	camera* cam;
	hitable_list *world;
	unsigned int nx;
	unsigned int ny;
	unsigned int ns;
	unsigned int max_depth;
	float min_attenuation;

	sample* samples;
	clr_rec* h_clrs;
	ray** h_rays;
	ray* d_rays;
	cu_hit* d_hits;
	clr_rec* d_clrs;
	sphere* d_scene;
	material* d_materials;
	bool init_rnds = true;

	pixel* pixels;
	float3* h_colors;

	clock_t kernel = 0;
	clock_t generate = 0;
	clock_t compact = 0;

private:
	void render_cycle(work_unit* wu1, work_unit* wu2);
	void copy_rays_to_gpu(const work_unit* wu);
	void start_kernel(const work_unit* wu);
	void copy_colors_from_gpu(const work_unit* wu);
	void compact_rays(work_unit* wu);

	uint total_rays;
	cudaStream_t stream;
	work_unit **wunits;
	uint next_pixel = 0;
	int remaining_pixels = 0;
	uint num_runs = 0;
	sphere *light_shape;
	int* pixel_idx;
	inline void generate_ray(work_unit* wu, int ray_idx, int x, int y);
};