# Neural Network Operating System

This repository contains the implementation of an operating system that supports neural network architecture using separate processes and threads on a multi-core processor. The system utilizes inter-process communication (IPC) through pipes for exchanging information such as weights and biases between processes. Each layer of the neural network is represented as a separate process, and each neuron within a layer is treated as a separate thread. The system leverages the processing power of multiple cores for parallel computation.

## Requirements

To run this operating system, you need a Linux-based system, preferably Ubuntu. The following system calls and libraries are used in the implementation:

- fork(): For creating child processes.
- pipes (Named/unnamed): For inter-process communication.
- pthread library: For creating and managing threads.
- mutexes and semaphores: For process synchronization.
- Memory management: Implemented using system memory allocation mechanisms.

## Usage

1. Clone the repository to your local machine:

   ```bash
   git clone https://github.com/awahab111/neural-os-net.git
   ```

2. Navigate to the project directory:

   ```bash
   cd neural-os-net
   ```

3. Compile the code using a C++ compiler:

   ```bash
   g++ p1.cpp -o neural_network -lpthread
   ```

4. Run the program:

   ```bash
   ./neural_network
   ```

## Architecture

The operating system implements a neural network with separate processes and threads. Each layer of the neural network is represented as a separate process, and each neuron within a layer is treated as a separate thread. The architecture involves the following components:

- **Layer Process:** Each layer is implemented as a separate process, responsible for processing a specific batch of input data. The layer process receives input data from the previous layer through a pipe and applies weights and biases to generate the output. The output is then passed to the next layer through another pipe.

- **Neuron Thread:** Each neuron within a layer is treated as a separate thread. These threads can be assigned to different cores of the processor to enable parallel processing of inputs.

- **Inter-Process Communication (IPC):** Pipes are used for inter-process communication to exchange information such as weights and biases between processes. The layer processes communicate with each other through these pipes.

- **Process Synchronization:** Synchronization primitives such as mutexes and semaphores are used to ensure that only one process accesses shared resources like weights and biases at any given time. This prevents race conditions and ensures consistency in the neural network's computation.

- **Memory Management:** The operating system provides mechanisms for allocating and deallocating memory for the neural network processes and threads. Each process and thread has access to its own memory space to avoid conflicts.

- **Process and Thread Management:** The system efficiently manages processes and threads to enable parallel processing of inputs and reduce the training time of the neural network. Process scheduling ensures smooth and efficient execution of the neural network training process.

## Contribution

Contributions to this project are welcome. You can contribute by opening issues for bug reports or feature requests, and by submitting pull requests with improvements to the code.

Please ensure that your contributions align with the goals of the project and follow the guidelines provided in the repository.

## License

This project is licensed under the [MIT License](LICENSE). Feel free to use, modify, and distribute the code for personal or commercial purposes. See the `LICENSE` file for more details.
