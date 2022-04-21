edge_detection:
	gcc -o edge_detection edge_detection.c -fopenmp -lm

clean:
	rm edge_detection