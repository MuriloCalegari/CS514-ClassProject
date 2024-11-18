# CS514-ClassProject

### Onboarding

Run the ./setup.sh file to install the necessary dependencies.

```
chmod +x setup.sh
./setup.sh
```

There's a launch.json configuration to run the project test.
Alternatively, you may run the ns3 file directly with:

```
cd ./ns-allinone-3.43/ns-3.43
./ns3 run adaptive-tcp-test
```

If you encounter any errors building the project, consider cleaning the build directory:

```
./ns3 clean
./ns3 configure --enable-mpi
```

### Changing the congestion control algorithm
You may locate the code for the CCA under the ns-3.43 folder
on /src/internet/model/adaptive-tcp.cc