# Face Benchmark

**face-benchmark** is a program to test the performance of the face system. It runs
on a node between n consumer nodes and n producer nodes. It creates a left face and
a right face for each pair of consumer node and producer node respectively. The left
and right faces in each pair are tied to each other at the Interest/Data/Nack layer,
which means that anything received on the left face is sent to the corresponding
right face, and anything received on the right face is sent to the corresponding
left face. The program passively waits for an incoming connection from one side which
triggers an outgoing connection to the other side. The FacePersistency is set to
"on-demand" for all incoming faces, and "persistent" for all outgoing faces.

The FaceUris for each face pair can be configured via a configuration file. Each
line of the configuration file consists of a left FaceUri and a right FaceUri
separated by a space. FaceUri schemes "tcp4" and "udp4" are supported. The left face
and right face are allowed to have different FaceUri schemes. All FaceUris MUST be
in canonical form.

Usage example:

1. Configure FaceUris in `face-benchmark.conf`
2. On the router node, run `./face-benchmark face-benchmark.conf`
3. Run NFD on the consumer/producer node pairs
