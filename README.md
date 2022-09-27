# OctopusMon: Network-wide Measurement with Reduced Overhead

This repository contains all related code of our paper "OctopusMon: Network-wide Measurement with Reduced Overhead". 

## Introduction

Network-wide measurement is essential to quickly locate and debug network problems. Existing works often have linear or sub-linear monitoring overhead. It is challenging to simultaneously achieve full-coverage to various network anomalies and consistently small monitoring overhead. This paper presents OctopusMon to accurately monitor network anomalies with reduced overhead, which is proportional to the scale of abnormal traffic. The key idea of OctopusMon is to first identify several abnormal flow groups at network edge, and then focuses on only monitoring the traffic in abnormal flow groups inside the network. At network edge, we propose an algorithm called OctopusSketch to perform coarse-grained measurements, i.e., quickly identifying the group/subnet of abnormal flows. Inside the network, we propose an algorithm called MicroSketch to perform fined-grained measurements, i.e., deeply inspecting each abnormal flow in a per-flow per-hop manner. We have fully deployed OctopusMon in a testbed built with 10 Tofino switches, and conduct large-scale simulations. The results show that OctopusMon achieves 4.4× and more than 10× higher accuracy than state-of-the-art LightGuardian and Marple in locating network anomalies. All related codes are open-sourced anonymously.

## About this repository

* `CPU` contains codes of OctopusSketch and MicroSketch implemented on CPU platforms. 

* `NSPY` contains codes implemneted on NS.PY simulation platform. 

* `testbed` contains codes related to our testbed. We have deployed OctopusMon on a testbed built with Edgecore Wedge 100BF-32X switches (with Tofino ASIC). 

* More details can be found in the folders.