

enum TOPOLOGY
{
	TOPOLOGY_TRIANGLE_LIST,
	TOPOLOGY_TRIANGLE_STRIP,
	TOPOLOGY_LINE_LIST,
	TOPOLOGY_LINE_STRIP,
	TOPOLOGY_POINTS_LIST,

	TOPOLOGIES_COUNT,
};

enum IE_FORMATS
{
	IE_FORMAT_TYPELESS,
	IE_FORMAT_U16,
	IE_FORMAT_U32,
	IE_FORMAT_3U32,
	IE_FORMAT_S32,
   IE_FORMAT_2S32,
	IE_FORMAT_F32,
	IE_FORMAT_2F32,
	IE_FORMAT_3F32,
	IE_FORMAT_4F32,
	IE_FORMAT_16F32,

	IE_FORMATS_COUNT,
};

enum FILL_MODES
{
	FILL_MODE_SOLID,
	FILL_MODE_WIREFRAME,
};

enum CULL_MODES
{
	CULL_MODE_NONE,
	CULL_MODE_BACK,
	CULL_MODE_FRONT,
};

#define OBJECT3D_STRUCTURE \
	u32 mesh_uid;\
	u32 texinfo_uid;\
	\
	V3 pos;\
	V3 scale;\
	Quaternion rotation;\
	Color color;

struct  Object3d{
	OBJECT3D_STRUCTURE

   void default()
   {
      scale = {1,1,1};
      color = {1,1,1,1};
      rotation = UNIT_QUATERNION;
   }

   void fill(
      u32 _mesh_uid,
      u32 _texinfo_uid,
      V3 _pos,
      V3 _scale,
      Quaternion _rotation,
      Color _color
   )
   {
      mesh_uid = _mesh_uid;
      texinfo_uid = _texinfo_uid;
      pos = _pos;
      scale = _scale;
      rotation = _rotation;
      color = _color;
   }
};



struct Camera{
   V3 pos;
	Quaternion rotation;
	V3 looking_direction; // this is most of the time normalized
	V3 euler_angles; // the only reason for this is to be able to clamp the camera rotation between a certain range
   // i want to learn how to clamp it just using quaternions but i have no idea
};

struct Face
{
   b32 skip_drawing;

   union
   {
      u16 vertex_indices [4];
      struct
      {
         u16 v00, v10, v01, v11;
      };
   };
};

struct Bone
{
   V3 pos;
   Quaternion dir;
};

struct Bone_joint_indices{
   u16 origin_joint;
   u16 target_joint;
};

struct Keyframe
{
   f32 time;
   Bone bone;
   //TODO: interpolation;
};

struct Animation
{
   f32 length;
   u16 keyframes_count;
   u16 bones_count;

   Keyframe* all_keyframes;
   u16* bones_keyframes_count;
   u16* bones_root_keyframe_index;
};

//@@constructor@@
struct Mesh_primitive
{
	void* vertices;
	u32 vertex_size;
	u32 vertex_count;

	u16* indices;
	u32 indices_count;
	u32 topology_uid;
	
	void fill(
		void* _vertices,
		u32 _vertex_size,
		u32 _vertex_count,
		u16* _indices,
		u32 _indices_count,
		u32 _topology_uid
	){
		vertices = _vertices;
		vertex_size = _vertex_size;
		vertex_count = _vertex_count;
		indices = _indices;
		indices_count = _indices_count;
		topology_uid = _topology_uid;
	}
};

struct Submesh
{
   u32 vertex_count;
   Vertex* vertices;

   u32 indices_count;
   u16* indices;
   TOPOLOGY topology_uid;
};

struct Mesh
{
   char** submeshes_names;
   Submesh* submeshes;
   u16 submeshes_count;
};


internal Quaternion
create_quaternion(V3 v, f32 angle)
{
   f32 sin_a = SINF(angle/2);
   f32 cos_a = COSF(angle/2);
   return v4_normalize({sin_a*v.x, sin_a*v.y, sin_a*v.z, cos_a});
}

// applies q1 to q2, or is the same as first applying q2 and then applying q1
internal Quaternion
multiply_quaternions(Quaternion q1, Quaternion q2)
{
   Quaternion result;

   // UNINITIALIZED QUATERNION

   result.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
   result.x = q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y;
   result.y = q1.w*q2.y - q1.x*q2.z + q1.y*q2.w + q1.z*q2.x;
   result.z = q1.w*q2.z + q1.x*q2.y - q1.y*q2.x + q1.z*q2.w;

   return result;
}

internal Quaternion
quaternion_transform_locally(Quaternion current_q, Quaternion transform_q)
{
   
   ASSERT((current_q.x || current_q.y || current_q.z || current_q.w ) && (transform_q.x || transform_q.y || transform_q.z || transform_q.w));
   return multiply_quaternions(transform_q, current_q);
}
internal Quaternion
quaternion_transform_globally(Quaternion current_q, Quaternion transform_q)
{
   ASSERT((current_q.x || current_q.y || current_q.z || current_q.w ) && (transform_q.x || transform_q.y || transform_q.z || transform_q.w));
   return multiply_quaternions(current_q,transform_q);
}

internal Quaternion
quaternion_invert(Quaternion q)
{
   return {-q.x,-q.y,-q.z,q.w};
}
internal Quaternion
lerp_quaternions(Quaternion a, Quaternion b, f32 t)
{
   Quaternion result;
   float dot = v4_dot(a,b);
   float t_inverse = 1.0f - t;
   if (dot < 0) {
      result.w = (t_inverse * a.w) + (t * -b.w);
      result.x = (t_inverse * a.x) + (t * -b.x);
      result.y = (t_inverse * a.y) + (t * -b.y);
      result.z = (t_inverse * a.z) + (t * -b.z);
   } else {
      result.w = (t_inverse * a.w) + (t * b.w);
      result.x = (t_inverse * a.x) + (t * b.x);
      result.y = (t_inverse * a.y) + (t * b.y);
      result.z = (t_inverse * a.z) + (t * b.z);
   }
   return v4_normalize(result);
}

// this function is probably insanely wrong, use lerp for now instead
// internal Quaternion
// slerp_quaternions(Quaternion a, Quaternion b, f32 t)
// {
//    Quaternion result;
//    float dot = (a.w * b.w) + (a.x * b.x) + (a.y * b.y) + (a.z * b.z);

//    ASSERT(-1.0f < dot && dot < 1.0f);
   
//    if (dot < 0.0) {
//       b.w = -b.w;
//       b.x = -b.x;
//       b.y = -b.y;
//       b.z = -b.z;
//    }

//    // this two lines are super dumb chatgpt
//    f32 theta = ACOSF(dot);
//    f32 sin_theta = SINF(theta);
//    // why the f is there an alpha here
//    f32 alpha = SINF((1-t)*theta / sin_theta);
//    f32 beta = SINF(t*theta) / sin_theta;

   
//    result.w = b.w * alpha + b.w * beta;
//    result.x = b.x * alpha + b.x * beta;
//    result.y = b.y * alpha + b.y * beta;
//    result.z = b.z * alpha + b.z * beta;

//    return result;
// }
internal Quaternion
slerp_quaternions(Quaternion q1, Quaternion q2, f32 t)
{   
   // Calculate dot product
   f32 dot = v4_dot(q1, q2);
   
   // Determine the direction of interpolation
   if(dot < 0.0f)
   {
      q2 = quaternion_invert(q2);
      dot = -dot;
   }

   // Ensure dot product is within bounds
   dot = MIN(1.0f, MAX(-1.0f, dot));
   
   // Calculate angle between quaternions
   f32 theta = acosf(dot);
   
   // Interpolate
   
   Quaternion interpolated = v4_addition((sinf((1 - t) * theta) / sinf(theta)) * q1, (sinf(t * theta) / sinf(theta)) * q2);
   
   // Normalize result
   interpolated  = v4_normalize(interpolated);
   
   return interpolated;
}

internal V4
v4_apply_quaternion(V4 v, Quaternion q)
{
   
   ASSERT( q.x || q.y || q.z || q.w );
   Quaternion inverse_q = quaternion_invert(q);
   return multiply_quaternions(multiply_quaternions(inverse_q, v), q);
}
internal V3
v3_apply_quaternion(V3 v, Quaternion q)
{
   return v4_apply_quaternion(v3_to_v4(v), q).v3;
}

// THIS WORKS BUT THE LOCAL ROTATION WILL PROBABLY BE WEIRD 
internal Quaternion
quaternion_from_v1_to_v2(V3 v1, V3 v2)
{   
   Quaternion result;
   V3 direction = v3_cross(v2, v1);
   if(direction.x || direction.y || direction.z || 
      ( 
         ((v2.x >= 0 && v1.x >= 0) || (v2.x <= 0 && v1.x <= 0)) && 
         ((v2.y >= 0 && v1.y >= 0) || (v2.y <= 0 && v1.y <= 0 )) && 
         ((v2.z >= 0 && v1.z >= 0) || (v2.z <= 0 && v1.z <= 0))
      )
   )
   {
      //TODO: i am not sure if i should normalize the direction alone before normalizing the whole quaternion 
      // but apperently no
      result.v3 = direction; 
      result.w = SQRT((v3_sqr_magnitude(v1) * v3_sqr_magnitude(v2))) + v3_dot(v1, v2);
      return v4_normalize(result);
   }
   else
   {
      return {0,SQRT_1HALF,SQRT_1HALF,0};
   }
}
internal Quaternion
quaternion_from_vector(V3 v)
{
   return quaternion_from_v1_to_v2({1,0,0}, v);
}

internal Quaternion
quaternion_from_v1_to_v2_keep_x(V3 v1, V3 v2)
{
   Quaternion result;
   V3 xz_v2 = {v2.x, 0, v2.z};
   V3 xz_v1 = {v1.x, 0, v1.z};
   result = quaternion_from_v1_to_v2(xz_v1,xz_v2);   
   result = quaternion_transform_globally(result, quaternion_from_v1_to_v2(xz_v2, v2));

   return result;
}

// functions to rotate vectors using quaternions
internal V3
v3_rotate_with_quaternion(V3 v,V3 rotation_v, f32 angle)
{
   f32 cos_angle = COSF(angle/2);
   f32 sin_angle = SINF(angle/2);

   Quaternion q = {sin_angle*rotation_v.x, sin_angle*rotation_v.y, sin_angle*rotation_v.z, cos_angle};
   Quaternion inverse_q = {-sin_angle*rotation_v.x, -sin_angle*rotation_v.y, -sin_angle*rotation_v.z, cos_angle};

   
   ASSERT( q.x || q.y || q.z || q.w );
   return multiply_quaternions(multiply_quaternions(q, v4(v.x,v.y,v.z,0)), inverse_q).v3;
}

// unused function
internal V4
v4_rotate_with_quaternion(V4 v, V3 rotation_v, f32 angle)
{
   f32 cos_angle = COSF(angle/2);
   f32 sin_angle = SINF(angle/2);

   Quaternion q = v4(sin_angle*rotation_v.x, sin_angle*rotation_v.y, sin_angle*rotation_v.z, cos_angle);
   Quaternion inverse_q = v4(-sin_angle*rotation_v.x, -sin_angle*rotation_v.y, -sin_angle*rotation_v.z, cos_angle);

   ASSERT( q.x || q.y || q.z || q.w );
   Quaternion temp_q = multiply_quaternions(q, v);
   temp_q = multiply_quaternions(temp_q, inverse_q);
   return temp_q;
}


internal Quaternion
euler_angles_to_quaternion(V3 angles) // roll (x), pitch (y), yaw (z), angles are in radians
{
   // Abbreviations for the various angular functions

   f32 cr = COSF(angles.x * 0.5f);
   f32 sr = SINF(angles.x * 0.5f);
   f32 cp = COSF(angles.y * 0.5f);
   f32 sp = SINF(angles.y * 0.5f);
   f32 cy = COSF(angles.z * 0.5f);
   f32 sy = SINF(angles.z * 0.5f);

   Quaternion q;
   q.w = cr * cp * cy + sr * sp * sy;
   q.x = sr * cp * cy - cr * sp * sy;
   q.y = cr * sp * cy + sr * cp * sy;
   q.z = cr * cp * sy - sr * sp * cy;

   return q;
}

//THIS FUNCTION IS PROBABLY WRONG
internal V3
quaternion_to_euler_angles(Quaternion q)
{
   V3 result;
   
   f32 sinx_cosy = 2*((q.w*q.x) + (q.y*q.z));
   f32 cosx_cosy = 1 - (2* (q.x*q.x + q.y*q.y));
   result.x = ATAN2F(sinx_cosy, cosx_cosy);

   f32 siny = SQRT( 1 + (2 * ((q.w*q.y) - (q.x-q.z)) ) );
   f32 cosy = SQRT( 1 - (2 * ((q.w*q.y) - (q.x-q.z)) ) );
   result.y = 2 * ATAN2F(siny, cosy) - (PI32/2);

   f32 sinz_cosy = 2 * ((q.w*q.z) + (q.x*q.y));
   f32 cosz_cosy = 1 - (2 * ((q.y*q.y) + (q.z*q.z)));
   result.z = ATAN2F(sinz_cosy, cosz_cosy);

   return result;
}

// MATRICES 
union Matrix2x2
{
   struct
   {
      f32 m00, m01,
      m10, m11;
   };
};

internal f32
matrix2_determinant(Matrix2x2 m)
{
   return m.m00*m.m11 - m.m10*m.m01;
}

union Matrix3x3
{
   struct
   {
      f32 m00, m01, m02,
      m10, m11, m12,
      m20, m21, m22;
   };
};

internal V3
matrix3_x_v3(Matrix3x3 m, V3 v)
{
   V3 result;
   // result.x = m.m00*v.x + m.m01*v.y + m.m02*v.z;
   // result.y = m.m10*v.x + m.m11*v.y + m.m12*v.z;
   // result.z = m.m20*v.x + m.m21*v.y + m.m22*v.z;
   
   result.x = m.m00*v.x + m.m10*v.y + m.m20*v.z;
   result.y = m.m01*v.x + m.m11*v.y + m.m21*v.z;
   result.z = m.m02*v.x + m.m12*v.y + m.m22*v.z;

   return result;
}

internal f32
matrix3_determinant(Matrix3x3 m)
{
   f32 det_1 = m.m00*matrix2_determinant({m.m11,m.m12,m.m21,m.m22});
   f32 det_2 = m.m01*matrix2_determinant({m.m10,m.m12,m.m20, m.m22});
   f32 det_3 = m.m02*matrix2_determinant({m.m10,m.m11,m.m20, m.m21});
   return det_1 - det_2 + det_3;
}

union Matrix
{
   struct
   {
      f32 m00,m01,m02,m03,
         m10,m11,m12,m13,
         m20,m21,m22,m23,
         m30,m31,m32,m33;
   };
   struct Row
   {
      f32 c0,c1,c2,c3;
   };
   struct 
   {
      Row r[4]; 
   };
};
static Matrix IDENTITY_MATRIX = {
   1,0,0,0,
   0,1,0,0,
   0,0,1,0,
   0,0,0,1
};

internal f32 
matrix_determinant(Matrix a)
{
   f32 det_1 = a.m00*matrix3_determinant({a.m11,a.m12,a.m13, 
                                          a.m21,a.m22,a.m23, 
                                          a.m31,a.m32,a.m33});

   f32 det_2 = a.m01*matrix3_determinant({a.m10,a.m12,a.m13, 
                                          a.m20,a.m22,a.m23, 
                                          a.m30,a.m32,a.m33});

   f32 det_3 = a.m02*matrix3_determinant({a.m10,a.m11,a.m13, 
                                          a.m20,a.m21,a.m23, 
                                          a.m30,a.m31,a.m33});

   f32 det_4 = a.m03*matrix3_determinant({a.m10,a.m11,a.m12, 
                                          a.m20,a.m21,a.m22, 
                                          a.m30,a.m31,a.m32});
   return det_1 - det_2 + det_3 - det_4;
}

internal Matrix
matrix_inverse(Matrix a)
{
   Matrix3x3 transposed_rotation = {
      a.m00, a.m10, a.m20,
      a.m01, a.m11, a.m21,
      a.m02, a.m12, a.m22
   };
   V3 inverse_translation = matrix3_x_v3(transposed_rotation, {a.m30,a.m31,a.m32});
   inverse_translation = {-inverse_translation.x, -inverse_translation.y, -inverse_translation.z};
   Matrix result = {
      transposed_rotation.m00, transposed_rotation.m01, transposed_rotation.m02, 0,
      transposed_rotation.m10, transposed_rotation.m11, transposed_rotation.m12, 0,
      transposed_rotation.m20, transposed_rotation.m21, transposed_rotation.m22, 0,
      inverse_translation.x, inverse_translation.y, inverse_translation.z, 1
   };

   return result;
}

internal V4
apply_transform(Matrix t, V4 v)
{
   V4 result;
   result.x = v.x*t.m00 + v.y*t.m10 + v.z*t.m20 + v.w*t.m30;
   result.y = v.x*t.m01 + v.y*t.m11 + v.z*t.m21 + v.w*t.m31;
   result.z = v.x*t.m02 + v.y*t.m12 + v.z*t.m22 + v.w*t.m32;
   result.w = v.x*t.m03 + v.y*t.m13 + v.z*t.m23 + v.w*t.m33;
   
   // result.x = v.x*t.m00 + v.y*t.m01 + v.z*t.m02 + v.w*t.m03;
   // result.y = v.x*t.m10 + v.y*t.m11 + v.z*t.m12 + v.w*t.m13;
   // result.z = v.x*t.m20 + v.y*t.m21 + v.z*t.m22 + v.w*t.m23;
   // result.w = v.x*t.m30 + v.y*t.m31 + v.z*t.m32 + v.w*t.m33;

   return result;
}

internal Matrix
matrix_from_quaternion(Quaternion q) {
   f32 xy = q.x * q.y;
   f32 xz = q.x * q.z;
   f32 xw = q.x * q.w;
   f32 yz = q.y * q.z;
   f32 yw = q.y * q.w;
   f32 zw = q.z * q.w;
   f32 xSquared = q.x * q.x;
   f32 ySquared = q.y * q.y;
   f32 zSquared = q.z * q.z;

   Matrix matrix;
   
   matrix.m00 = 1 - 2 * (ySquared + zSquared); 
   matrix.m01 = 2 * (xy - zw);
   matrix.m02 = 2 * (xz + yw);
   matrix.m03 = 0;

   matrix.m10 = 2 * (xy + zw);
   matrix.m11 = 1 - 2 * (xSquared + zSquared);
   matrix.m12 = 2 * (yz - xw);
   matrix.m13 = 0;

   matrix.m20 = 2 * (xz - yw);
   matrix.m21 = 2 * (yz + xw);
   matrix.m22 = 1 - 2 * (xSquared + ySquared);
   matrix.m23 = 0;

   matrix.m30 = 0;
   matrix.m31 = 0;
   matrix.m32 = 0;
   matrix.m33 = 1;

   return matrix;
}

internal Matrix
matrix_multiplication(Matrix a, Matrix b)
{
   Matrix result;
   result.m00 = a.m00*b.m00 + a.m01*b.m10 + a.m02*b.m20 + a.m03*b.m30;
   result.m01 = a.m00*b.m01 + a.m01*b.m11 + a.m02*b.m21 + a.m03*b.m31;
   result.m02 = a.m00*b.m02 + a.m01*b.m12 + a.m02*b.m22 + a.m03*b.m32;
   result.m03 = a.m00*b.m03 + a.m01*b.m13 + a.m02*b.m23 + a.m03*b.m33;
   
   result.m10 = a.m10*b.m00 + a.m11*b.m10 + a.m12*b.m20 + a.m13*b.m30;
   result.m11 = a.m10*b.m01 + a.m11*b.m11 + a.m12*b.m21 + a.m13*b.m31;
   result.m12 = a.m10*b.m02 + a.m11*b.m12 + a.m12*b.m22 + a.m13*b.m32;
   result.m13 = a.m10*b.m03 + a.m11*b.m13 + a.m12*b.m23 + a.m13*b.m33;
   
   result.m20 = a.m20*b.m00 + a.m21*b.m10 + a.m22*b.m20 + a.m23*b.m30;
   result.m21 = a.m20*b.m01 + a.m21*b.m11 + a.m22*b.m21 + a.m23*b.m31;
   result.m22 = a.m20*b.m02 + a.m21*b.m12 + a.m22*b.m22 + a.m23*b.m32;
   result.m23 = a.m20*b.m03 + a.m21*b.m13 + a.m22*b.m23 + a.m23*b.m33;
   
   result.m30 = a.m30*b.m00 + a.m31*b.m10 + a.m32*b.m20 + a.m33*b.m30;
   result.m31 = a.m30*b.m01 + a.m31*b.m11 + a.m32*b.m21 + a.m33*b.m31;
   result.m32 = a.m30*b.m02 + a.m31*b.m12 + a.m32*b.m22 + a.m33*b.m32;
   result.m33 = a.m30*b.m03 + a.m31*b.m13 + a.m32*b.m23 + a.m33*b.m33;

   return result;
}

internal Matrix
operator*(Matrix a, Matrix b)
{
   return matrix_multiplication(a, b);
}


internal Matrix
build_perspective_matrix(f32 aspect_ratio, f32 fov, f32 nearz, f32 farz, f32 depth_effect)
{
   Matrix matrix;

   f32 inverse_tan = 1/(TANF(fov/2));
   //TODO: depth effect seems to do exactly the same as the fov
   // divided by aspect ratio cuz this needs to be h/w
   matrix.m00= inverse_tan/aspect_ratio;
   matrix.m01= 0;
   matrix.m02= 0;
   matrix.m03= 0;
   
   matrix.m10= 0;
   matrix.m11= inverse_tan;
   matrix.m12= 0;
   matrix.m13= 0;
   
   matrix.m20= 0;
   matrix.m21= 0;
   matrix.m22= depth_effect*farz/(farz-nearz);
   matrix.m23= depth_effect;
   
   matrix.m30= 0;
   matrix.m31= 0;
   matrix.m32= depth_effect*(-farz*nearz) / (farz-nearz);
   matrix.m33= 0;

   return matrix;
}

internal Matrix
build_orthographic_matrix(f32 aspect_ratio, f32 height, f32 nearz, f32 farz)
{
   f32 width = aspect_ratio*height;
   f32 depth = farz-nearz;
   Matrix matrix;
   
   matrix.m00= 2.0f/width;
   matrix.m01= 0;
   matrix.m02= 0;
   matrix.m03= 0;
   
   matrix.m10= 0;
   matrix.m11= 2.0f/height;
   matrix.m12= 0;
   matrix.m13= 0;
   
   matrix.m20= 0;
   matrix.m21= 0;
   // matrix.m22= -2/depth;
   matrix.m22 = 1 / (depth); // xmmath version of orthographic matrix
   matrix.m23= 0;
   
   matrix.m30= 0;
   matrix.m31= 0;
   // matrix.m32= -(farz+nearz)/(farz-nearz);
   matrix.m32 = nearz / (nearz - farz); // xmmath version of orthographic matrix
   // this is the same as 
   // matrix.m32 = - nearz / (depth);
   matrix.m33= 1;

   return matrix;
}

// i don't know what this is
// internal void
// build_orthographic_matrix_reference(f32 matrix[4][4], f32 aspect_ratio, f32 nearz, f32 farz)
// {aspect_ratio;
//    f32 left, right, top, bottom, width, height, depth;

//    left = -2;
//    right = 2;
//    top = 2;
//    bottom = -2;

//    width = right - left;
//    height =  top - bottom;
//    depth = farz - nearz;


//    // matrix[0][0]= width/2;
//    // matrix[0][1]= 0;
//    // matrix[0][2]= 0;
//    // matrix[0][3]= 0;
   
//    // matrix[1][0]= 0;
//    // matrix[1][1]= height/2;
//    // matrix[1][2]= 0;
//    // matrix[1][3]= 0;
   
//    // matrix[2][0]= 0;
//    // matrix[2][1]= 0;
//    // matrix[2][2]= depth/-2;
//    // matrix[2][3]= 0;
   
//    // matrix[3][0]= (left+right)/2;
//    // matrix[3][1]= (top+bottom)/2;
//    // matrix[3][2]= -(farz+nearz)/2;
//    // matrix[3][3]= 1;

//    matrix[0][0]= 2/width;
//    matrix[0][1]= 0;
//    matrix[0][2]= 0;
//    matrix[0][3]= 0;
   
//    matrix[1][0]= 0;
//    matrix[1][1]= 2/height;
//    matrix[1][2]= 0;
//    matrix[1][3]= 0;
   
//    matrix[2][0]= 0;
//    matrix[2][1]= 0;
//    matrix[2][2]= -2/depth;
//    matrix[2][3]= 0;
   
//    matrix[3][0]= -(right+left)/width;
//    matrix[3][1]= -(top+bottom)/height;
//    matrix[3][2]= -(farz+nearz)/depth;
//    matrix[3][3]= 1;


//    // matrix[0][0]= 1.0f/aspect_ratio;
//    // matrix[0][1]= 0;
//    // matrix[0][2]= 0;
//    // matrix[0][3]= 0;
   
//    // matrix[1][0]= 0;
//    // matrix[1][1]= 1.0f;
//    // matrix[1][2]= 0;
//    // matrix[1][3]= 0;
   
//    // matrix[2][0]= 0;
//    // matrix[2][1]= 0;
//    // matrix[2][2]= -0.01f;
//    // // matrix[2][2]= farz/(farz-nearz);
//    // matrix[2][3]= 0;
   
//    // matrix[3][0]= 0;
//    // matrix[3][1]= 0;
//    // matrix[3][2]= 0.01f;
//    // // matrix[3][2]= (-farz*nearz) / (farz-nearz);
//    // matrix[3][3]= 1.0f;

// }

internal Matrix
matrix_translation(V3 v)
{
   Matrix result = {0};
   result.m00 = 1;
   result.m11 = 1;
   result.m22 = 1;
   result.m33 = 1;

   result.m30 = v.x;
   result.m31 = v.y;
   result.m32 = v.z;
   return result;
}

internal Matrix
matrix_scale(V3 v)
{
   Matrix result = {0};
   result.m00 = v.x;
   result.m11 = v.y;
   result.m22 = v.z;
   result.m33 = 1;
   return result;
}


// THESE TWO ARE PRACTICALLY THE SAME, BUT I HAVE NOT INCLUDED A TEXRECT INTO THE INSTANCING DATA
struct Object_buffer_data
{
   Matrix transform;
   Color color;
   Rect_float texrect;
};


struct Instance_data
{
	Matrix object_transform;
	Color color;
   Rect_float texrect;
};

internal Matrix
calculate_object_transform(V3 pos, V3 scale, Quaternion rotation)
{
   return matrix_scale(scale) * matrix_from_quaternion(rotation) * matrix_translation(pos);
}