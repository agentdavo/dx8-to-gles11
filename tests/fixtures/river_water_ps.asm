ps.1.1
tex t0
tex t1
tex t2
tex t3
mul r0, v0, t0
mul r1, t1, t2
add r0, r0, t3
mul r0, r0, t3
add r0, r0, r1
