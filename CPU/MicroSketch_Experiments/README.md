# Codes of MicroSketch


- We emulate and test the performance of our MicroSketch using CAIDA2018 dataset.


## File Descriptions

- ``packet_interval_distribution`` contains the codes to calculate Average Absolute Error on top-k flow packet inter-arrival time estimation of MicroSketch.

## How to Run

- Replace `filename` in `trace.h` with the path of your data. 
- Run the following command.

```bash
$ make
$ ./main
```

