
#define SQRT(a) fast_sqrt(a)
#define ABS(x) math_abs(x) // TODO: wouldn't this be better if i inlined it?
#define SINF(a) sinf(a)
#define COSF(a) cosf(a)
#define TANF(a) tanf(a)
#define ASINF(a) asinf(a)
#define ACOSF(a) acosf(a)
#define ATANF(a) atanf(a)
#define ATAN2F(y,x) atan2f(y,x)

internal s32 math_max(s32 n1, s32 n2){return ((n1 < n2) ? n2 :n1);}
internal u32 math_max(u32 n1, u32 n2){return ((n1 < n2) ? n2 :n1);}
internal f32 math_max(f32 n1, f32 n2){return ((n1 < n2) ? n2 :n1);}

internal s32 math_min(s32 n1, s32 n2){return ((n1 < n2) ? n1 : n2);}
internal u32 math_min(u32 n1, u32 n2){return ((n1 < n2) ? n1 : n2);}
internal f32 math_min(f32 n1, f32 n2){return ((n1 < n2) ? n1 : n2);}

internal f32
f32_lerp(f32 a, f32 b, f32 t)
{
    return a*(1.0f-t) + b*t;
}

internal f32
r32_pow(f32 n, u32 e)
{
    f32 p = 1;
    while(e>0)
    {
        p = p*n;
        e--;
    }
    return p;
}

internal u32
u32_pow(u32 base, u32 exponent)
{
    ASSERT(exponent <= 64);
    u32 result = 1;
    while(exponent > 0)
    {
        result *= base;
        exponent--;
    }
    return result;
}

internal f32 math_abs(f32 n) {return ((n > 0) ? n : -1*n);} 
internal s32 math_abs(s32 n) {return ((n > 0) ? n : -1*n);}

// sqrt 

// john carmack's fast square root
internal f32
fast_sqrt(f32 n)
{
    ASSERT(n >= 0);
    f32 x = n * 0.5f;
    f32 y = n; // just to not use the address of the parameter i guess

    s32 i = *(s32*)&y; // casting the float to an integer
    i = 0x5f3759df - (i >> 1); // this is complete sorcery
    y = *(float*)&i; // re casting to a float

    y = y * (1.5f - (x * y * y)); // newton pass
    y = y * (1.5f - (x * y * y)); // optional second newton pass for more precision

    
    return n*y;
}

// newton's algorithm for square root
internal f32
newton_sqrt(f32 n)
{
    ASSERT(n >= 0);
    f32 r = n/2;
    f32 precision = n/1000;
    for(u32 i=0;(ABS(n-(r*r)) > precision && i < 10); i++)
    {
        r = (r+(n/r))/2;
    }
    return r;
}


struct V2
{
    f32 x;
    f32 y;
};
internal V2
v2(f32 x, f32 y)
{
    return {x,y};
}
internal V2
v2_addition(V2 v1, V2 v2)
{
    return {v1.x + v2.x, v1.y + v2.y};
}

internal V2
v2_difference(V2 v1, V2 v2)
{
    return {v1.x - v2.x, v1.y - v2.y};
}

internal V2
operator +(V2 v1, V2 v2)
{
    return v2_addition(v1, v2);
}

internal V2
operator -(V2 v1, V2 v2)
{
    return v2_difference(v1, v2);
}

internal V2
operator /(V2 v1, f32 a)
{
    return {v1.x/a, v1.y/a};
}
//Escalar multiplication
internal V2
v2_scalar_product(f32 e, V2 v){
    return {e*v.x, e*v.y};
}
internal V2
v2_scalar_product(s32 e, V2 v){
    return {e*v.x, e*v.y};
}
internal V2
v2_component_wise_product(V2 a, V2 b)
{
    return {a.x*b.x, a.y*b.y};
}
internal V2
operator *(f32 e, V2 v)
{
    return {e * v.x, e * v.y};
}
internal V2
operator *(s32 e, V2 v)
{
    return {e * v.x, e * v.y};
}
internal V2
operator *(V2 v, f32 e)
{
    return {e * v.x, e * v.y};
}
internal V2
operator *(V2 v, u32 e)
{
    return {e * v.x, e * v.y};
}
internal b8
operator ==(V2 v1, V2 v2)
{
    return(v1.x == v2.x && v1.y == v2.y);
}
internal b8
operator !=(V2 v1, V2 v2)
{
    return(v1.x != v2.x || v1.y != v2.y);
}

struct Int2
{
    s32 x;
    s32 y;
};

internal Int2
int2(int a, int b)
{
    return {a, b};
}
internal Int2
v2_to_int2(V2 v)
{
    return {(int)v.x, (int)v.y};
}

internal Int2
int2_addition(Int2 a, Int2 b)
{
    return {a.x+b.x, a.y+b.y};
}
internal b32
operator !=(Int2 i1, Int2 i2)
{
    return !(i1.x == i2.x && i1.y == i2.y);
}
internal Int2
operator +(Int2 i1, Int2 i2){
    return {i1.x+i2.x, i1.y+i2.y};
}
internal Int2
operator -(Int2 i1, Int2 i2){
    return {i1.x-i2.x, i1.y-i2.y};
}
internal V2
operator /(Int2 v, f32 d){
    return {v.x/d, v.y/d};
}
internal V2
operator *(f32 d, Int2 v){
    return {d*v.x, d*v.y};
}
internal Int2
operator *(int d, Int2 v)
{
    return {d*v.x, d*v.y};
}

internal f32 v2_magnitude(V2 v){return SQRT(v.x*v.x + v.y*v.y);}
internal f32 v2_magnitude(f32 x, f32 y){return v2_magnitude({x,y});}
internal f32 int2_magnitude(Int2 v){return v2_magnitude({(f32)v.x, (f32)v.y});}


internal V2 v2_normalize(V2 v){
    f32 vlength = v2_magnitude(v);
    if(vlength) return v/vlength;
    return {0,0};
}

internal V2 v2_normalize(f32 x, f32 y){return v2_normalize({x,y});}
internal V2 int2_normalize(Int2 v){return v2_normalize((f32)v.x, (f32)v.y);}

internal f32
v2_dot(V2 v1, V2 v2){
    return (v1.x*v2.x) + (v1.y*v2.y);
}

internal V2
v2_min(V2 a, V2 b)
{
    f32 min_x = MIN(a.x, b.x);
    f32 min_y = MIN(a.y, b.y);
    return {min_x, min_y};
}
internal V2
v2_max(V2 a, V2 b)
{
    f32 max_x = MAX(a.x, b.x);
    f32 max_y = MAX(a.y, b.y);
    return {max_x, max_y};
}

internal V2
v2_lerp (V2 a, V2 b, f32 t)
{
    return {
        f32_lerp(a.x,b.x,t),
        f32_lerp(a.y,b.y,t)
    };
}

internal V2
v2_project_a_on_b(V2 a, V2 b)
{
    f32 b_magnitude = v2_magnitude(b);
    ASSERT(b_magnitude);
    V2 normalized_b = b/b_magnitude;
    f32 dot = v2_dot(a,normalized_b);
    return dot*normalized_b;
}

struct Int3
{
    int x;
    int y;
    int z;
};
internal Int3
operator +(Int3 a, Int3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

internal Int3
int3(int x, int y, int z)
{
    return {x,y,z};
}
internal bool
operator ==(Int3 a, Int3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

struct uInt3
{
    u32 x,y,z;
};
internal uInt3
uint3(u32 x, u32 y, u32 z)
{
    return {x,y,z};
}
internal uInt3
int3_to_uint3(Int3 i)
{
    return {(u32)i.x,(u32)i.y,(u32)i.z};
}

internal uInt3
operator +(uInt3 a, uInt3 b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

union V3
{
    struct {
        f32 x;
        f32 y;
        f32 z;
    };
    struct {
        f32 r;
        f32 g;
        f32 b;
    };
    Int3 i3;
    V2 v2;
};
static V3 NULL_V3 = {0,0,0};
internal V3
v3(f32 x, f32 y, f32 z)
{
    return {x,y,z};
}
internal Int3
v3_to_int3(V3 v)
{
    return {(int)floorf(v.x), (int)floorf(v.y), (int)floorf(v.z)};
}
internal V3
v3_invert(V3 v)
{
    return {-v.x, -v.y, -v.z};
}

internal V3
v3_per_component_multiplication(V3 a, V3 b)
{
    return {a.x*b.x, a.y*b.y, a.z*b.z};
}

internal f32
v3_dot(V3 v1, V3 v2){
    return (v1.x*v2.x) + (v1.y*v2.y) + (v1.z*v2.z);
}
internal V3
v3_cross(V3 u, V3 v)
{
    return {
        (u.y*v.z) - (u.z*v.y),
        (u.z*v.x) - (u.x*v.z),
        (u.x*v.y) - (u.y*v.x)
    };
}

internal V3
v3_addition(V3 v1, V3 v2){
    return {v1.x+v2.x, v1.y+v2.y, v1.z+v2.z};
}
internal V3
operator +(V3 v1, V3 v2){
    return v3_addition(v1, v2);
}
internal b32
operator ==(V3 v1, V3  v2){
    return (v1.x==v2.x && v1.y==v2.y && v1.z==v2.z);
}

internal V3
v3_difference(V3 v1, V3 v2){
    V3 result = {0};
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    // {v1.x-v2.x, v1.y-v2.y, v1.z-v2.z}
    return result;
}
internal V3
operator -(V3 v)
{
    return {-v.x, -v.y, -v.z};
}
internal V3
operator -(V3 v1, V3 v2){
    return v3_difference(v1, v2);
}
internal V3
v3_multiply(f32 e, V3 v){
    return {e * v.x, e * v.y, e*v.z};
}
internal V3
operator *(f32 e, V3 v){
    return v3_multiply(e, v);
}
internal V3
operator /(V3 v, f32 x){
    return {v.x / x, v.y / x, v.z /x};
}

internal f32
v3_sqr_magnitude(V3 v){
    return (v.x*v.x + v.y*v.y + v.z*v.z);
}

internal f32 
v3_magnitude(V3 v){return SQRT(v.x*v.x + v.y*v.y + v.z*v.z);}
internal f32 
v3_magnitude(f32 x, f32 y, f32 z){return v3_magnitude({x, y, z});}

internal V3 v3_normalize(V3 v){
    f32 vlength = v3_magnitude(v);
    if(vlength) return (v/vlength);
    return {0,0,0};
}
internal V3
v3_normalize(f32 x, f32 y, f32 z){return v3_normalize({x, y, z});}

internal V3
v3_lerp(V3 a, V3 b, f32 t)
{
    return {
        f32_lerp(a.x,b.x,t),
        f32_lerp(a.y,b.y,t),
        f32_lerp(a.z,b.z,t)
    };
}

struct Int4
{
    int x;
    int y;
    int z;
    int w;
};

union V4
{   
    struct{
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };
    Int4 i;
    V2 v2;
    V3 v3;
};

typedef V4 Quaternion;
static Quaternion UNIT_QUATERNION = {0,0,0,1};
internal V4
operator+(V4 v1, V4 v2)
{
    return {v1.x+v2.x,v1.y+v2.y,v1.z+v2.z,v1.w+v2.w};
}

internal V4
v4(f32 x, f32 y, f32 z, f32 w)
{
    return {x,y,z,w};
}

internal f32
v4_magnitude(V4 v)
{
    return SQRT((v.x*v.x) + (v.y*v.y) + (v.z*v.z) + (v.w*v.w));
}

internal V4
v4_normalize(V4 v)
{
    f32 vlength = v4_magnitude(v);
    return {v.x/vlength, v.y/vlength, v.z/vlength, v.w/vlength};
}
internal V4
v4_addition(V4 a, V4 b)
{
    return {a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w};
}

internal f32
v4_dot(V4 v1, V4 v2)
{
    return (v1.x*v2.x) + (v1.y*v2.y) + (v1.z*v2.z) + (v1.w*v2.w);
}

internal V4
operator *(f32 a, V4 v)
{
    return {a*v.x, a*v.y, a*v.z, a*v.w};
}


internal V4
v3_to_v4(V3 v)
{
   return {v.x,v.y,v.z,0};
}


internal f32
v2_angle(V2 v)
{
    return ATAN2F(v.y, v.x);
}
internal f32
v2_angle(f32 x, f32 y){return v2_angle({x,y});}

union Rect_int
{
    struct{
        s32 x;
        s32 y;
        s32 w;
        s32 h;
    };
    struct{
        Int2 pos;
        Int2 size;
    };
};
internal Rect_int
rect_int(Int2 pos, Int2 size)
{
    return {pos.x, pos.y, size.x, size.y};
}
internal b32
point_vs_rect_int(Int2 p, Rect_int rect)
{
	b32 x_inside = rect.pos.x <=  p.x && p.x  < rect.pos.x + rect.size.x;
	b32 y_inside = rect.pos.y <=  p.y && p.y  < rect.pos.y + rect.size.y;
	return x_inside && y_inside;
} 

union Rect_float
{
    struct {
        V2 pos;
        V2 size;
    };
    struct {
        f32 x, y, w, h;
    };
};
internal b32
point_vs_rect_float(V2 p, Rect_float rect)
{
	b32 x_inside = rect.pos.x <=  p.x && p.x  < rect.pos.x + rect.size.x;
	b32 y_inside = rect.pos.y <=  p.y && p.y  < rect.pos.y + rect.size.y;
	return x_inside && y_inside;
}
internal Rect_float
operator *(f32 scalar, Rect_float rect)
{
    return {rect.x*scalar, rect.y*scalar, rect.w*scalar, rect.h*scalar};
}
internal Rect_float
operator /(Rect_float rect, f32 scalar)
{
    return {rect.x/scalar, rect.y/scalar, rect.w/scalar, rect.h/scalar};
}

#define RECT_CENTER( pos, size) ((pos) + ((size)/2))
#define ENTITY_CENTER(entity) ((entity.pos) + ((entity.size)/2))

internal f32
v2_angle_between(V2 v1, V2 v2)
{
    f32 dot = v2_dot(v1,v2);
    f32 magnitudes = v2_magnitude(v1) * v2_magnitude(v2);

    if(COMPARE_FLOATS(dot, -magnitudes)){
        return PI32;
    }

    if(!magnitudes)
    {
        return 0;
    }
    f32 cos_angle = dot / magnitudes;
    
    f32 result = ACOSF(cos_angle);
    if(isnan(result))
    {
        ASSERT(true);
    }
    return result;
}

internal bool
rect_vs_rect(Rect_float r1, Rect_float r2)
{
    return r1.x+F32_MIN_THRESHOLD < r2.x+r2.w
        && r2.x+F32_MIN_THRESHOLD < r1.x+r1.w
        && r1.y+F32_MIN_THRESHOLD < r2.y+r2.h
        && r2.y+F32_MIN_THRESHOLD < r1.y+r1.h;
        ;
}

internal bool
int_rect_vs_rect(Rect_int r1, Rect_int r2)
{
    return r1.x < r2.x+r2.w
        && r2.x < r1.x+r1.w
        && r1.y < r2.y+r2.h
        && r2.y < r1.y+r1.h
        ;
}

internal f32
snap_to_grid(f32 value, f32 delta)
{
    f32 difference = value - (f32)((s32)(value/delta));
    return value - difference;
}
internal V3
line_vs_plane(V3 line_0, V3 normalized_line_d, V3 plane_p, V3 plane_normal)
{
    f32 dot1 = v3_dot(plane_normal, plane_p - line_0);
    f32 dot2 = v3_dot(plane_normal, normalized_line_d);
    f32 t = dot1/dot2;

    return (line_0 + (t*normalized_line_d));

}

internal V3
line_intersect_y0(V3 line_0, V3 line_d){
    f32 t = (-line_0.y / line_d.y);
    f32 x = line_0.x + (t*line_d.x);
    f32 z = line_0.z + (t*line_d.z);
    return {x,0,z};
}

internal V3 
line_intersect_z0(V3 line_0, V3 line_d)
{
    f32 t = (-line_0.z / line_d.z);
    f32 x = line_0.x + (t*line_d.x);
    f32 y = line_0.y + (t*line_d.y);
    return {x,y,0};
}

bool line_vs_sphere(V3 line_0, V3 line_v, V3 sphere_center, f32 sphere_radius, f32* closest_t) {
    f32 a = r32_pow(line_v.x, 2) + r32_pow(line_v.y, 2) + r32_pow(line_v.z, 2);
    f32 b = 2 * (line_v.x * (line_0.x - sphere_center.x) +
                    line_v.y * (line_0.y - sphere_center.y) +
                    line_v.z * (line_0.z - sphere_center.z));
    f32 c = r32_pow(line_0.x - sphere_center.x, 2) +
               r32_pow(line_0.y - sphere_center.y, 2) +
               r32_pow(line_0.z - sphere_center.z, 2) -
               r32_pow(sphere_radius, 2);

    f32 discriminant = r32_pow(b, 2) - 4 * a * c;

    b32 result = false;
    if (0 <= discriminant)
    {
        result = true;

        f32 discriminant_sqrt = SQRT(discriminant);

        f32 t1 = (-b + discriminant_sqrt) / (2 * a);
        f32 t2 = (-b - discriminant_sqrt) / (2 * a);

        *closest_t = MIN(t1,t2);
    }
    return result;
}

bool ray_vs_sphere(V3 line_0, V3 line_v, V3 sphere_center, f32 sphere_radius, V3* closest_point) {
    f32 a = r32_pow(line_v.x, 2) + r32_pow(line_v.y, 2) + r32_pow(line_v.z, 2);
    f32 b = 2 * (line_v.x * (line_0.x - sphere_center.x) +
                    line_v.y * (line_0.y - sphere_center.y) +
                    line_v.z * (line_0.z - sphere_center.z));
    f32 c = r32_pow(line_0.x - sphere_center.x, 2) +
               r32_pow(line_0.y - sphere_center.y, 2) +
               r32_pow(line_0.z - sphere_center.z, 2) -
               r32_pow(sphere_radius, 2);

    f32 discriminant = r32_pow(b, 2) - 4 * a * c;

    b32 result = false;
    if (0 <= discriminant)
    {
        f32 discriminant_sqrt = SQRT(discriminant);

        f32 t1 = (-b + discriminant_sqrt) / (2 * a);
        f32 t2 = (-b - discriminant_sqrt) / (2 * a);

        if( t1 > 0 && t2 > 0)
        {
            result = true;
            f32 t = MIN(t1,t2);
            *closest_point = {
                line_0.x + t*line_v.x,
                line_0.y + t*line_v.y,
                line_0.z + t*line_v.z
            };
        }
    }
    return result;
}

/*  returns a vector that represents the distance between 
    the closest point of the box to the center of the sphere
    Check if the magnitude is less than or equal to the sphere radius */
internal V3
sphere_vs_box(V3 sc, V3 bmin, V3 bmax){
    // Calculate the closest point on the box to the sphere
    V3 closest_point;
    closest_point.x = MAX(bmin.x, MIN(sc.x, bmax.x));
    closest_point.y = MAX(bmin.y, MIN(sc.y, bmax.y));
    closest_point.z = MAX(bmin.z, MIN(sc.z, bmax.z));

    // Calculate the distance between the closest point and the sphere center
    V3 distance = sc-closest_point;
    return distance;
}

// return value = overlap, if 0 <= overlap  then they don't overlap
internal f32 
sphere_vs_sphere(V3 c1,f32 r1, V3 c2, f32 r2){
    return ((r1+r2) - v3_magnitude(c1-c2));
}

internal f32
line_vs_triangle(V3 line_0, V3 line_v, V3 t1, V3 t2, V3 t3, V3* intersection_pos)
{
    V3 edge1, edge2, h, s, q;
    f32 a, f, u, v;

    edge1 = t2-t1;
    edge2 = t3-t1;

    h = v3_cross(line_v, edge2);
    a = v3_dot(edge1, h);
    if(COMPARE_FLOATS(a, 0)){
        return 0; // line is parallel
    }
    
    f = 1.0f / a;
    s = line_0 - t1;
    
    u = f * v3_dot(s, h);

    if(u < 0 || u > 1.0f){
        return 0;
    }
    
    q = v3_cross(s, edge1);
    v = f*v3_dot(line_v, q);

    if(v < 0 || (u + v) > 1.0f){
        return 0;
    }

    f32 t = f * v3_dot(edge2, q);

    *intersection_pos = line_0 + (t*line_v);

    return t;
}

// this doesn't work
#if 0
internal b32
point_vs_rect(V3 point, V3 p1, V3 p2, V3 p3, V3 p4)
{
    V3 v1 = p1-point;
    V3 v2 = p2-point;
    V3 v3 = p3-point;
    V3 v4 = p4-point;

    V3 cross1 = v3_cross(v1, v2);
    V3 cross2 = v3_cross(v2, v3);
    V3 cross3 = v3_cross(v3, v4);
    V3 cross4 = v3_cross(v4, v1);

    b32 is_inside = 
        (v3_dot(cross1, cross2) >= 0) &&
        (v3_dot(cross2, cross3) >= 0) &&
        (v3_dot(cross3, cross4) >= 0)
    ;

    return is_inside;
}

internal b32
line_vs_rect(V3 line_0, V3 line_v, V3 p1, V3 p2, V3 p3, V3 p4, V3* intersection_point)
{
    V3 edge1 = p2 - p1;
    V3 edge2 = p3 - p1;

    V3 normal = v3_cross(edge1, edge2);

    f32 d = -normal.x*p1.x - normal.y*p1.y - normal.z*p1.z;

    f32 t = -(normal.x * line_0.x + normal.y * line_0.y + normal.z * line_0.z + d) /
        (normal.x * line_v.x + normal.y * line_v.y + normal.z * line_v.z);
    if (t<0){
        return 0;
    }

    *intersection_point = line_0 + (t*(line_v));

    if(point_vs_rect(*intersection_point, p1,p2,p3,p4))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
#endif


// returns the tvalue that the line needs to advance to intersect with the box
internal float 
line_vs_aabb(V3 line_p, V3 line_d, V3 box_min, V3 box_max) 
{
    float temp_t1 = (box_min.x - line_p.x) / line_d.x;
    float temp_t2 = (box_max.x - line_p.x) / line_d.x;

    float t_min = MIN(temp_t1, temp_t2);
    float t_max = MAX(temp_t1, temp_t2);

    temp_t1 = (box_min.y - line_p.y) / line_d.y;
    temp_t2 = (box_max.y - line_p.y) / line_d.y;

    float ty_min = MIN(temp_t1, temp_t2);
    float ty_max = MAX(temp_t1, temp_t2);

    if ((t_min > ty_max) || (ty_min > t_max))
        return -1;


    if (ty_min > t_min)
        t_min = ty_min;

    if (ty_max < t_max)
        t_max = ty_max;
        
    temp_t1 = (box_min.z - line_p.z) / line_d.z;
    temp_t2 = (box_max.z - line_p.z) / line_d.z;

    float tz_min = MIN(temp_t1, temp_t2);
    float tz_max = MAX(temp_t1, temp_t2);


    if ((t_min > tz_max) || (tz_min > t_max))
        return -1;

    if (tz_min > t_min)
        t_min = tz_min;

    if (tz_max < t_max)
        t_max = tz_max;

    if (t_min < 0 && t_max < 0)
        return -1;

    return t_min; // Intersection exists
}

internal V2
v2_rotate(V2 vector, f32 angle){
    f32 cos_angle = COSF(angle);
    f32 sin_angle = SINF(angle);

    return {
        vector.x * cos_angle - vector.y * sin_angle,
        vector.x * sin_angle + vector.y * cos_angle
    };
}

// Function to rotate a vector using a rotation matrix
internal V3 
v3_rotate_x(V3 vector, f32 angle) 
{
    f32 cos_angle = COSF(angle);
    f32 sin_angle = SINF(angle);
    
    V3 result = { 
        vector.x,
        vector.y * cos_angle - vector.z * sin_angle,
        vector.y * sin_angle + vector.z * cos_angle
    };
    return result;
}
    
    
internal V3
v3_rotate_y(V3 vector, f32 angle)
{
    f32 cos_angle = COSF(angle);
    f32 sin_angle = SINF(angle);
    
    V3 result = { 
        vector.x * cos_angle + vector.z * sin_angle,
        vector.y,
        -vector.x * sin_angle + vector.z * cos_angle
    };
    return result;
}

internal V3
v3_rotate_z(V3 vector, f32 angle)
{
    f32 cos_angle = COSF(angle);
    f32 sin_angle = SINF(angle);
    
    V3 result = { 
        vector.x * cos_angle - vector.y * sin_angle,
        vector.x * sin_angle + vector.y * cos_angle,
        vector.z
    };

    return result;
}

internal f32
get_shortest_angle_difference(f32 target_angle, f32 current_angle)
{
    f32 result = target_angle - current_angle;
    if( result < (-PI32))
    {
        result += TAU32;
    }
    else if (PI32 < result)
    {
        result -= TAU32;
    }

    return result;
}

//TODO: move this to helpers

internal f32
rng_lcg(u32 seed){
    return ((f32)((8121 * seed + 28411) % 134456) / 134456);
}

struct RNG{
    u32 last_seed;

    f32 lcg(f32 max_value){
        u32 bits_flipped = (last_seed ^ 0xffffffff); // this is my own addition so i may be introducing some problem i don't know
        last_seed =  ((8121 * bits_flipped + 28411) % 134456);
        return max_value*(f32)last_seed / 134456;
    }
    
    f32 next(f32 max_value){
        u32 temp = (last_seed << 13) ^ last_seed;
        ++last_seed;
        return max_value*((f32)((temp * (temp * temp * 15731 + 789221) + 1376312589) & 0x7fffffff) / 0x7fffffff);
    }

    // this dice tries to produce n amount of successes each second
    f32 time_dice(f32 hits_per_second, f32 delta_time){
        f32 success_rate = hits_per_second*delta_time;

        return next(1) < success_rate;
    }
};

internal f32
rng_rand(u32 seed){
    seed = (seed << 13) ^ seed;
    return (f32)((seed * (seed * seed * 15731 + 789221) + 1376312589) & 0x7fffffff) / 0x7fffffff;
}


internal f32
f32_ease_in_out_quad(f32 t)
{
    return t < 0.5f ? 2*t*t : 1- ((-2*t+2)*(-2*t+2)/2);
}

internal f32
sample_2d_perlin_noise(float* noisemap, 
    int width, int height, int x, int y, 
    u32 max_iterations, //default 8
    int initial_sample_resolution, // default 2 and a multiple of 2
    float influence_step_multiplier // default .5f
    )
{
    float amplitude = 1.0f;

    int step_count = initial_sample_resolution;
    float cumulative_value = 0;
    float cumulative_amplitude = 0;

    UNTIL(current_i, max_iterations)
    {
        int xstep_size = width/step_count;
        int ystep_size = height/step_count;

        if(!xstep_size || !ystep_size) break;

        int ix1 = (x/xstep_size)*xstep_size;
        int iy1 = (y/ystep_size)*ystep_size;
        int ix2 = (((x+xstep_size)/xstep_size)*xstep_size)%width;
        int iy2 = (((y+ystep_size)/ystep_size)*ystep_size)%height;

        float samplex1y1 = noisemap[iy1*width + ix1];
        float samplex2y1 = noisemap[iy1*width + ix2];
        float samplex1y2 = noisemap[iy2*width + ix1];
        float samplex2y2 = noisemap[iy2*width + ix2];

        float tx = ((float)x-ix1)/xstep_size;
        float ty = ((float)y-iy1)/ystep_size;

        float lerped_y1 = f32_lerp(samplex1y1, samplex2y1, tx);
        float lerped_y2 = f32_lerp(samplex1y2, samplex2y2, tx);

        cumulative_value += amplitude * f32_lerp(lerped_y1, lerped_y2, ty);
        cumulative_amplitude += amplitude;
        
        amplitude = influence_step_multiplier*amplitude;
        step_count *= 2;
    }

    return cumulative_value/cumulative_amplitude;
}

internal f32
sample_3d_perlin_noise(float* noisemap, 
    int width, int height, int depth, 
    int x, int y, int z,
    u32 max_iterations, int initial_sample_resolution, float influence_step_multiplier
    )
{
    float amplitude = 1.0f;

    int step_count = initial_sample_resolution;
    float cumulative_value = 0;
    float cumulative_amplitude = 0;

    UNTIL(current_i, max_iterations)
    {
        int xstep_size = width/step_count;
        int ystep_size = height/step_count;
        int zstep_size = depth/step_count;

        if(!xstep_size || !ystep_size || !zstep_size) break;

        int ix1 = (x/xstep_size)*xstep_size;
        int iy1 = (y/ystep_size)*ystep_size;
        int iz1 = (z/zstep_size)*zstep_size;
        int ix2 = (((x+xstep_size)/xstep_size)*xstep_size)%width;
        int iy2 = (((y+ystep_size)/ystep_size)*ystep_size)%height;
        int iz2 = (((z+zstep_size)/zstep_size)*zstep_size)%depth;

        float samplex1y1z1 = noisemap[iz1*width*height + iy1*width + ix1];
        float samplex2y1z1 = noisemap[iz1*width*height + iy1*width + ix2];
        float samplex1y2z1 = noisemap[iz1*width*height + iy2*width + ix1];
        float samplex2y2z1 = noisemap[iz1*width*height + iy2*width + ix2];
        float samplex1y1z2 = noisemap[iz2*width*height + iy1*width + ix1];
        float samplex2y1z2 = noisemap[iz2*width*height + iy1*width + ix2];
        float samplex1y2z2 = noisemap[iz2*width*height + iy2*width + ix1];
        float samplex2y2z2 = noisemap[iz2*width*height + iy2*width + ix2];

        float tx = ((float)x-ix1)/xstep_size;
        float ty = ((float)y-iy1)/ystep_size;
        float tz = ((float)z-iz1)/zstep_size;

        float lerped_y1z1 = f32_lerp(samplex1y1z1, samplex2y1z1, tx);
        float lerped_y2z1 = f32_lerp(samplex1y2z1, samplex2y2z1, tx);
        float lerped_y1z2 = f32_lerp(samplex1y1z2, samplex2y1z2, tx);
        float lerped_y2z2 = f32_lerp(samplex1y2z2, samplex2y2z2, tx);

        float lerped_z1 = f32_lerp(lerped_y1z1, lerped_y2z1, ty);
        float lerped_z2 = f32_lerp(lerped_y1z2, lerped_y2z2, ty);

        

        cumulative_value += amplitude * f32_lerp(lerped_z1, lerped_z2, tz);
        cumulative_amplitude += amplitude;
        
        amplitude = influence_step_multiplier*amplitude;
        step_count *= 2;
    }

    return cumulative_value/cumulative_amplitude;
}


union Box
{
    struct 
    {
        u32 left, top, front, right, bottom, back;
    };
    struct 
    {
        uInt3 min, max;
    };
    
};
internal Box
box(u32 l, u32 t, u32 f, u32 r, u32 bt, u32 bk)
{
    return {l,t,f,r,bt,bk};
}

internal float 
sdf_capsule( V3 p, V3 a, V3 b, float r )
{
  V3 pa = p - a; 
  V3 ba = b - a;
  float h = CLAMP(0.0f, v3_dot(pa,ba)/v3_dot(ba,ba), 1.0f);
  return v3_magnitude( pa - (h*ba) ) - r;
}
