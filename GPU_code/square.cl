__kernel void square(__global double *input, __global double *output)
{
  size_t id = get_global_id(0);
  output[id] = input[id] * input[id];
}
