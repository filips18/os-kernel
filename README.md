# os-kernel
This project represents a simple, yet fully functioning, operating system kernel with complete multithreading support and time-sharing, written in C++. 
# Intro
In this project, a subsystem of this kernel is realized - a subsystem for thread control. This subsystem contains the concepts of threading, semaphores, events and time-sharing, with an added implementation of "signals" between threads as a way of communication.
The kernel is realized in such a way where the user application and the kernel itself share an address space.In other words, they represent the same program.In the development of this kernel only the basic C/C++ libraries have been used.

The source code is meant to be executed on a Windows 32-bit platform(any Windows OS that is 32-bit will work), and it is written on a Borland C version 3.1 (from 1992) for a i8086 processor.The documentation for the C version and the i8086 processor are provided in the "Documentation" folder.The required files for running this code are given in the "Borland 3.1" folder.
# Thread
The subsystem for thread control, some of the concepts realized are:
- creating and starting a thread
- context switching and preemption (when a thread requests it, or an interrupt happens, or the time allocated for execution of that thread has expired)
- standard semaphores
- standard events (in other words, a binary semaphore)

The Thread, Semaphore and Event classes are the wrapper classes for the kernel's implementation of those concepts.They are used by the user as a way to work with these concepts.This project allows the creation of an unlimited amount of user threads (the actual limit is the physical limit of the available memory).The class PCB represents the actual implementation of a thread inside the kernel, where the link between a user thread and a kernel thread is "1 to 1".
# Semaphore
This is a basic implementation of a semaphore, with "wait" and "signal" operations.
The "signal" function is different from the usual semaphore implementation of this concept because it has 1 argument, if it is greater than 0 the semaphore should unblock this amount of threads.The value of the semaphore always increases by the value of the argument, unless it is 0 in which case the "signal" operation behaves in a usual way.If there are less threads to unblock than the argument demands, then return the actual number of threads unblocked.
The "wait" function is different from the usual semaphore implementation because it also has 1 argument, the argument represents the maximum amount of time that the thread will be blocked.If this argument is 0, the wait time of a thread is not limited.

The KernelSem class represents the actual implementation of the semaphore inside the kernel.In other words, the Semaphore class is just a wrapper class that the user uses to operate with semaphores.
# Event
This is an implementation of a standard event concept, which can be described as a binary semaphore where only one thread can be blocked (the thread which actually made that event object), and the semaphore is being signalized through an interrupt.Using this concept the sporadic/periodic thread can be realized.Every time an interrupt happens which signals the event, the context is switched (unless it is not allowed to by the system).The unblocked thread that waited on the event is not guaranteed to start executing after the interrupt happens, that decision is made by the "Scheduler" and is not in control of the source code.
# Signals
This is an implementation of the concept of asynchronous signals which threads can send to each other.Any thread can signal any other thread, or the system can send a signal to any thread.The functions used for processing these signals are called "signal handlers", these can be registered to be called afterward when processing and handling a signal that they have been registered to.

A thread can receive a signal at any moment, and all signals should be handled as soon as possible.There are 16 possible signals, with IDs between 0 and 15.
A signal can be blocked globally for all threads or locally for one thread.While the signal is blocked it should not be processed and handled.A thread inherits all necessary settings from it's parent thread (the thread that was running at the time of creation of the child thread).

Signal handler for the signal 0 is a system function which kills a thread abruptly, thus releasing all the resources it has occupied.The handler is implemented by the source code and all other signal handlers registered for the signal 0 will not execute.

Signals with ID's 1 and 2 are sent by the sistem to a given thread, signal 1 is sent to a thread when a child thread has finished executing, and the signal 2 is sent to a thread when it finishes executing.
