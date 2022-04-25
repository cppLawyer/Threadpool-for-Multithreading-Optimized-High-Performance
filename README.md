# Threadpool-for-Multithreading


One thread will be used to launch other threads very cheaply, and if there is no task,
the one thread will yield and execute another task. With this threadpool, 
it is possible to stop at any moment and restart at any moment It is also 
possible to let the system decide when to deallocate the threadpool object.


- The threadpool uses an inbuild queue.

- Easy to Use.

- Low level optimized.

- Thread limit checker, so there are no performance penalties.

- Only for basic functions with no return type and no parameters, 
Please let me know if those features are required.
