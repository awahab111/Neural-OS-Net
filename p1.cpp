#include <fstream>
#include <sstream>
#include <pthread.h>
#include <string>
#include <semaphore.h>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define NUM_OF_INPUTS 2
using namespace std;

sem_t sem;
sem_t completion_sem;
int threadCountSeekg = 0;
int WeightsSeekg = 0;
bool finalLayer = false;
int worker_threads = 0;

struct Neuron
{
    int thread_number;
    double inputs;
    double *output;
    double result;
};

int count_threads()
{
    int num_of_threads = 0;
    string line;
    ifstream fin("weights_and_inputs.txt");
    fin.seekg(threadCountSeekg);

    // counting the number of lines
    while (getline(fin, line))
    {
        if (line == "")
            break;
        num_of_threads++;
    }
    threadCountSeekg = fin.tellg();
    if (fin.eof())
        finalLayer = true;
    fin.close();
    return num_of_threads;
}

int count_weights()
{
    int num_of_weights = 0;
    string line;
    ifstream fin("weights_and_inputs.txt");
    fin.seekg(WeightsSeekg);
    getline(fin, line);
    stringstream iss(line);
    while (getline(iss, line, ','))
    {
        num_of_weights++;
    }
    fin.close();
    return num_of_weights;
}

void read_weights(vector<double> &weights)
{
    string line, temp;
    ifstream fin("weights_and_inputs.txt");

    fin.seekg(WeightsSeekg);
    getline(fin, line);
    stringstream iss(line);
    while (getline(iss, temp, ','))
    {
        weights.push_back(stod(temp));
    }
    // updating the seekg value for the next thread
    WeightsSeekg = fin.tellg();
    fin.close();
}

void *thread_func(void *arg)
{
    // add the linear combination of inputs and weights
    sem_wait(&sem);
    Neuron neuron = *(Neuron *)arg;
    vector<double> weights;
    read_weights(weights);

    for (int i = 0; i < weights.size(); i++)
    {
        neuron.output[i] = neuron.inputs * weights[i];
    }
    cout << "Thread " << neuron.thread_number << " output: " << endl;
    for (int i = 0; i < weights.size(); i++)
    {
        cout << neuron.output[i] << " ";
    }
    cout << endl;
    sem_post(&sem);
    worker_threads++;
    pthread_exit(NULL);
}

void generate_input(double &output, double new_inputs[2])
{
    new_inputs[0] = ((output * output) + output + 1) / 2;
    new_inputs[1] = ((output * output) - output) / 2;
}

void readPipeInput(const int &NUM_OF_THREADS, int &numOfInputs, vector<double> &inputs, int &read_end_f)
{
    // read from pipe
    inputs.resize(numOfInputs);
    for (int i = 0; i < NUM_OF_THREADS; i++)
    {
        read(read_end_f, &inputs[i], sizeof(double));
    }
}

void writePipeInput(const int &NUM_OF_WEIGHTS, double *result, int fd_2[2])
{
    if (!finalLayer)
    {
        for (int i = 0; i < NUM_OF_WEIGHTS; i++)
        {
            write(fd_2[1], &result[i], sizeof(double));
        }
    }
}

double combineFinalOutput(const int &NUM_OF_WEIGHTS, double *result)
{
    cout << "-----------Final Layer---------" << endl;
    double output = 0;
    for (int i = 0; i < NUM_OF_WEIGHTS; i++)
    {
        output += result[i];
    }
    cout << "Final Output: " << output << endl;
    return output;
}

double *final_layer(const int &NUM_OF_WEIGHTS, int &reading_end_b, int &writing_end_b, double *result)
{
    double *new_inputs = NULL;
    if (finalLayer)
    {
        double output = combineFinalOutput(NUM_OF_WEIGHTS, result);

        new_inputs = new double[2];
        generate_input(output, new_inputs);

        // sending to back process
        close(reading_end_b);
        for (int i = 0; i < 2; i++)
        {
            if (write(writing_end_b, &new_inputs[i], sizeof(double)))
                cout << "Successfully wrote to pipe" << endl;
            else
                cout << "Error writing to pipe" << endl;
        }
        close(writing_end_b);
    }
    return new_inputs;
}

double *hidden_layer(const int &process_num, int fd_1[2], int &reading_end_b, int &writing_end_b)
{
    double *new_inputs = NULL;
    if (!finalLayer)
    {
        // receiving from front process
        new_inputs = new double[2];

        close(fd_1[1]);
        for (int i = 0; i < 2; i++)
        {
            if (read(fd_1[0], &new_inputs[i], sizeof(double)))
                cout << "Input " << new_inputs[i] << " from P " << process_num << endl;
            else
                cout << "Error reading from pipe" << endl;
        }
        close(fd_1[0]);

        // sending to back process
        if (process_num != 0)
        {
            close(reading_end_b);
            for (int i = 0; i < 2; i++)
            {
                write(writing_end_b, &new_inputs[i], sizeof(double));
            }
            close(writing_end_b);
        }
    }
    return new_inputs;
}

void* completionFunc(void *arg)
{
    int NUM_OF_THREADS = *(int *)arg;
    while(worker_threads != NUM_OF_THREADS);
    cout << "worker_threads: " << worker_threads << endl;
    sem_post(&completion_sem);
    pthread_exit(NULL);
}

double *process_inputs(const int &NUM_OF_THREADS, const int &NUM_OF_WEIGHTS, vector<double> &inputs)
{
    Neuron *neurons = new Neuron[NUM_OF_THREADS];
    pthread_t *threads = new pthread_t[NUM_OF_THREADS];
    for (int i = 0; i < NUM_OF_THREADS; i++)
    {
        neurons[i].thread_number = i;
        neurons[i].inputs = inputs[i];
        neurons[i].output = new double[NUM_OF_WEIGHTS];
        pthread_create(&threads[i], NULL, thread_func, (void *)(&neurons[i]));
        sleep(1);
    }

    pthread_t completion_thread;
    pthread_create(&completion_thread, NULL, completionFunc, (void*)&NUM_OF_THREADS);

    sem_wait(&completion_sem);
    //resetting worker_threads value for another call
    worker_threads = 0;


    double *result = new double[NUM_OF_WEIGHTS]{0.0};
    for (int i = 0; i < NUM_OF_WEIGHTS; i++)
    {
        for (int j = 0; j < NUM_OF_THREADS; j++)
        {
            result[i] += neurons[j].output[i];
        }
    }
    return result;
}

int main(int argc, char *argv[])
{
    sem_init(&completion_sem, 0, 0);
    worker_threads = 0;

    int process_num = 0;
    int seekg = 0;
    int numOfInputs = 2;
    int reading_end_b = 0;
    int writing_end_b = 0;
    int read_end_f = 0;
    int write_end_f = 0;

    if (argc > 1)
    {
        process_num = atoi(argv[1]);
    }
    if (argc > 2)
    {
        seekg = atoi(argv[2]);
    }
    if (argc >= 3)
    {
        numOfInputs = atoi(argv[3]);
    }
    if (argc >= 4)
    {
        reading_end_b = atoi(argv[4]);
    }
    if (argc >= 5)
    {
        writing_end_b = atoi(argv[5]);
    }
    if (argc >= 6)
    {
        read_end_f = atoi(argv[6]);
    }
    if (argc >= 7)
    {
        write_end_f = atoi(argv[7]);
    }

    cout << "Process number: " << process_num << endl;

    WeightsSeekg = threadCountSeekg = seekg;
    const int NUM_OF_THREADS = count_threads();
    sem_init(&sem, 0, 1);
    const int NUM_OF_WEIGHTS = count_weights();
    const int outputSize = NUM_OF_WEIGHTS;
    double *result = new double[NUM_OF_WEIGHTS]{0.0};

    cout << "Number of threads: " << NUM_OF_THREADS << endl;

    // enter inputs
    vector<double> inputs;
    if (process_num == 0)
    {
        double temp = 0.0;
        cout << "Enter inputs:" << endl;
        for (int i = 0; i < NUM_OF_THREADS; i++)
        {
            cin >> temp;
            inputs.push_back(temp);
        }
    }

    else
    {
        close(write_end_f);
        readPipeInput(NUM_OF_THREADS, numOfInputs, inputs, read_end_f);
    }

    result = process_inputs(NUM_OF_THREADS, NUM_OF_WEIGHTS, inputs);

    WeightsSeekg = threadCountSeekg;

    int fd_1[2]; //back propagation
    int fd_2[2]; //forward propagation
    if (!finalLayer)
    {
        pipe(fd_1);
        pipe(fd_2);
    }

    if (fork() == 0)
    {
        if (!finalLayer)
            execlp("./p1", "./p1", to_string(++process_num).c_str(), to_string(WeightsSeekg).c_str(), to_string(outputSize).c_str(), to_string(fd_1[0]).c_str(), to_string(fd_1[1]).c_str(), to_string(fd_2[0]).c_str(), to_string(fd_2[1]).c_str(), NULL);
        else // if final process
        {
            exit(0);
        }
    }

    close(fd_2[0]);
    writePipeInput(NUM_OF_WEIGHTS, result, fd_2);

    double *new_inputs;
    if (finalLayer)
        new_inputs = final_layer(NUM_OF_WEIGHTS, reading_end_b, writing_end_b, result);
    else
        new_inputs = hidden_layer(process_num, fd_1, reading_end_b, writing_end_b);

    // ---- re-front propagation ----
    if (process_num == 0)
    {
        // delete inputs
        for (int i = 0; i < inputs.size(); i++)
        {
            inputs.pop_back();
        }
        // adding new inputs
        for (int i = 0; i < NUM_OF_THREADS; i++)
        {
            inputs.push_back(new_inputs[i]);
        }
    }
    else
    {
        // delete inputs
        for (int i = 0; i < inputs.size(); i++)
        {
            inputs.pop_back();
        }
        cout << "-------------Process number: " << process_num << " is now readying." << endl;
        readPipeInput(NUM_OF_THREADS, numOfInputs, inputs, read_end_f);
        close(read_end_f);
        cout << "~~~~~~~~~~~~~Process number: " << process_num << " has now read" << endl;
    }
    delete[] result;
    WeightsSeekg = seekg;
    result = process_inputs(NUM_OF_THREADS, NUM_OF_WEIGHTS, inputs);

    cout << "=============== Process number: " << process_num << endl;

    writePipeInput(NUM_OF_WEIGHTS, result, fd_2);
    close(fd_2[1]);

    if (finalLayer)
    {
        combineFinalOutput(NUM_OF_WEIGHTS, result);
    }
    cout << "Process number: " << process_num << " is now exiting." << endl;
    return 0;
}
