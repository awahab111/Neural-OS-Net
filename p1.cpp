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
using namespace std;

sem_t sem;
int threadCountSeekg = 0;
int WeightsSeekg = 0;
bool finalLayer = false;

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
    cout << "Thread " << pthread_self() << " output:" << endl;
    for (int i = 0; i < weights.size(); i++)
    {
        cout << neuron.output[i] << " ";
    }
    cout << endl;
    sem_post(&sem);
    pthread_exit(NULL);
}

void generate_input(double &output, double new_inputs[2])
{
    new_inputs[0] = ((output*output) + output + 1)/2;
    new_inputs[1] = ((output*output) - output)/2;
}

int main(int argc, char *argv[])
{
    int process_num = 0;
    int seekg = 0;
    int numOfInputs = 2;
    int reading_end = 0;
    int writing_end = 0;

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
        reading_end = atoi(argv[4]);
    }
    if (argc >= 5)
    {
        writing_end = atoi(argv[5]);
    }

    cout << "Process number: " << process_num << endl;

    WeightsSeekg = threadCountSeekg = seekg;
    sem_init(&sem, 0, 1);
    const int NUM_OF_THREADS = count_threads();
    const int NUM_OF_WEIGHTS = count_weights();
    const int outputSize = NUM_OF_WEIGHTS;
    Neuron *neurons = new Neuron[NUM_OF_THREADS];
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
        cout << "reading" << endl;
        // read from pipe
        inputs.resize(numOfInputs);
        int fd = open("pipe", O_RDONLY);
        for (int i = 0; i < NUM_OF_THREADS; i++)
        {
            // sleep(1);
            double thread_input;
            read(fd, &thread_input, sizeof(double));
            inputs[i] = thread_input;
            cout << thread_input << endl;
        }

        // read(fd, &inputs, numOfInputs*sizeof(double));
        close(fd);
    }

    pthread_t *threads = new pthread_t[NUM_OF_THREADS];
    for (int i = 0; i < NUM_OF_THREADS; i++)
    {
        neurons[i].thread_number = i;
        neurons[i].inputs = inputs[i];
        neurons[i].output = new double[NUM_OF_WEIGHTS];
        pthread_create(&threads[i], NULL, thread_func, (void *)(&neurons[i]));
        pthread_join(threads[i], NULL);
    }

    WeightsSeekg = threadCountSeekg;

    for (int j = 0; j < NUM_OF_WEIGHTS; j++)
    {
        for (int i = 0; i < NUM_OF_THREADS; i++)
        {
            result[j] += neurons[i].output[j];
        }
        // cout << result[j] << endl;
    }
    int fd[2];
    if(!finalLayer)
    pipe(fd);

    if (fork() == 0)
    {
        if (!finalLayer)
            execlp("./p1", "./p1", to_string(++process_num).c_str(), to_string(WeightsSeekg).c_str(), to_string(outputSize).c_str(), to_string(fd[0]).c_str(), to_string(fd[1]).c_str(), NULL);
        else //if final process
        {
          //  mkfifo("output_pipe", 0666);
            exit(0);
        }
    }
    if (!finalLayer)
    {
        mkfifo("pipe", 0666);
        int fd = open("pipe", O_WRONLY);
        for (int i = 0; i < NUM_OF_WEIGHTS; i++)
        {
            write(fd, &result[i], sizeof(double));
        }
        close(fd);
    }
    double output = 0;
    if (finalLayer)
    {
        cout << "-----------Final Layer---------" << endl;
        double output;
        for (int i = 0; i < NUM_OF_WEIGHTS; i++)
        {
            output += neurons[i].output[i];
        }
        cout << "Final Output: " << output << endl;

        double new_inputs[2];
        generate_input(output, new_inputs);
        //close reading end
        close(reading_end);
        for(int i = 0; i < 2; i++)
        {
            if(write(writing_end, &new_inputs[i], sizeof(double)))
            {
                cout << "Successfully wrote to pipe" << endl;
            }
            else{
                cout << "Error writing to pipe" << endl;
            }
        }
        close(writing_end);

    }
    if (!finalLayer)
    {
        //receiving from front process
        double new_inputs[2];
        close(fd[1]);
        for(int i = 0; i < 2; i++)
        {
            if(read(fd[0], &new_inputs[i], sizeof(double)))
            {
                cout << "Input " << new_inputs[i] << " from P " << process_num << endl;
            }
            else
            {
                cout << "Error reading from pipe" << endl;
            }
        }

        close(fd[0]);
        //sending to back process
        if(process_num != 0)
        {
            close(reading_end);
            for(int i = 0; i < 2; i++)
            {            
                write(writing_end, &new_inputs[i], sizeof(double));
            }
            close(writing_end);
        }
    }
    
    return 0;
   
}
