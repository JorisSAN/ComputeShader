#version 450 core

layout (local_size_x = 256) in;

uniform vec3 start = vec3(0.0);
uniform vec3 force = vec3(0.0);
uniform vec3 gravity = vec3(0.0, -1.0, 0.0);
uniform float timestep = 0.4;
uniform float totalTime = 0.0;

struct flock_member
{
    vec3 position;
    vec3 velocity;
    vec3 timeSpawn;
};

layout (std430, binding = 0) readonly buffer members_in
{
    flock_member member[];
} input_data;

layout (std430, binding = 1) buffer members_out
{
    flock_member member[];
} output_data;

shared flock_member shared_member[gl_WorkGroupSize.x];

void main(void)
{
    uint i, j;
    int global_id = int(gl_GlobalInvocationID.x);

    flock_member me = input_data.member[global_id];
    flock_member new_me;
    vec3 accelleration = vec3(0.0);

    new_me.position = me.position + me.velocity * timestep;
    new_me.timeSpawn = me.timeSpawn;
    accelleration += force + gravity * (totalTime - me.timeSpawn.x);
    accelleration += vec3(sin(global_id) * 0.05, 0.0, cos(global_id) * 0.05);
    new_me.velocity = me.velocity + accelleration * timestep;
    if (length(new_me.velocity) > 10.0)
        new_me.velocity = normalize(new_me.velocity) * 10.0;
    new_me.velocity = mix(me.velocity, new_me.velocity, 0.4);
    output_data.member[global_id] = new_me;
}
