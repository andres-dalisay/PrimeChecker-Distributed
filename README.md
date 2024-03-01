# PrimeChecker-Distributed

This is a distributed version of the PrimeChecker project made for STDISCM Programming Report 1.

## Group Members
- Dalisay, Andres
- Llamado, Jon
- Salvador, Bryce

## Details & Options
- If running using three separate machines, run each project (PrimeChecker-Client, PrimeChecker-Server, PrimeChecker-Slave) in each machine. Make sure that each machine is connected to the same network and is discoverable.
- Set the `MASTER_SERVER_IP` in both the Slave and Client to the IPv4 address of the machine running the PrimeChecker-Server.
- You can set the `THREAD_COUNT` and `MAX_BUFFER_SIZE` in both the Server and Slave to your desired number of threads and input limit respectively.
- The Client has the options `DISPLAY_ARRAY` and `SORT_ARRAY` to validate the contents of the resulting primes vector.