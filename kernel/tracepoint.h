#ifndef _TRACEPOINT_H
#define _TRACEPOINT_H

static struct synth_field_desc sched_fields[] = {
    {.type = "pid_t", .name = "next_pid_field"},
    {.type = "char[16]", .name = "next_comm_field"},
    {.type = "u64", .name = "ts_ns"},
    {.type = "u64", .name = "ts_ms"},
    {.type = "unsigned int", .name = "cpu"},
    {.type = "char[64]", .name = "my_string_field"},
    {.type = "int", .name = "my_int_field"},
};



#endif /* _TRACEPOINT_H */