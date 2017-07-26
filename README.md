# NS3

See details in compiler to know how to compile the files.

How to compile?
With dynamic library and header files correctly installed, the simulation models can be compiled with the following command.


g++ TheFileName.cc -O3 -o OutputName -I/usr/local/include/ns3.26 -I/usr/include/python2.7 -L /usr/local/lib/ \
-lns3.26-core -lns3.26-network \
-lns3.26-point-to-point -lns3.26-internet \
-lns3.26-applications -lns3.26-mobility -lns3.26-lte \
-lns3.26-buildings -lns3.26-config-store -lpython2.7 


TheFileName is the name of the file you want to compile. OutputName is for the binary execution file.

To run simulation:

cd directory
./OutputName

Current Progress
https://docs.google.com/document/d/1bWtdDEN6p_RcdBFJtv6qjwpMwerWEJaqQnSvuIz3cHs/edit?usp=sharing
