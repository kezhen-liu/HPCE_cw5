__kernel void kernel_xy(const float dx, const float dy, const unsigned maxIter, const float c_x, const float c_y, __global unsigned* dest){
	
	uint x=get_global_id(0);
    uint y=get_global_id(1);
	uint w=get_global_size(0);
	
	// Map pixel to z_0
	// complex_t z(-1.5f+x*dx, -1.5f+y*dy);
	float z_x = -1.5f+x*dx, z_y = -1.5f+y*dy

	//Perform a julia iteration starting at the point z_0, for offset c.
	//   z_{i+1} = z_{i}^2 + c
	// The point escapes for the first i where |z_{i}| > 2.

	unsigned iter=0;
	while(iter<maxIter){
		//if(abs(z) > 2){
		if(hypot(z_x, z_y) > 2)
			break;
		}
		// Anybody want to refine/tighten this?
		//z = z*z + input->c;
		z_x = z_x*z_x - z_y*z_y + c_x;
		z_y = 2*z_x*z_y + c_y;
		++iter;
	}
	dest[y*w+x] = iter;
}