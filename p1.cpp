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
    double* output;
};

int count_threads()
{
    int num_of_threads=0;
    string line;
    ifstream fin("weights_and_inputs.txt");
    fin.seekg(threadCountSeekg);

    //counting the number of lines
    while(getline(fin, line))
    {   
        if(line == "") break;
        num_of_threads++;
    }
    threadCountSeekg = fin.tellg();
    if(fin.eof())
        finalLayer = true;
    fin.close();
    return num_of_threads;
}

int count_weights()
{
    int num_of_weights=0;
    string line;
    ifstream fin("weights_and_inputs.txt");
    fin.seekg(WeightsSeekg);
    getline(fin, line);
    stringstream iss(line);
    while(getline(iss, line, ','))
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
        //updating the seekg value for the next thread
        WeightsSeekg = fin.tellg();
        fin.close();
}

//--------------
//PROBLEM: only one thread is able to read the weights at a time
//----------------

void* thread_func(void* arg)
{
    //add the linear combination of inputs and weights
    Neuron neuron = *(Neuron*)arg;
    vector<double> weights;
    sem_wait(&sem);
    read_weights(weights);
    sem_post(&sem);

    for(int i=0; i<weights.size(); i++)
    {
        neuron.output[i] = neuron.inputs * weights[i];
    }
    cout << "Thread " << pthread_self() << " output:" << endl;
    for(int i=0; i<weights.size(); i++)
    {
        cout << neuron.output[i] << " ";
    }
    cout << endl;
    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
   int process_num = 0;
   int seekg = 0;
   int numOfInputs = 2;

   if (argc > 1) {
      process_num = atoi(argv[1]);
   }
   if (argc > 2) {
      seekg = atoi(argv[2]);
   }
   if (argc >= 3) {
      numOfInputs = atoi(argv[3]);
   }

   cout << "process_num = " << process_num << endl;
   cout << "seekg = " << seekg << endl;
   cout << "numOfInputs = " << numOfInputs << endl;




    cout << "Process number: " << process_num << endl;

    WeightsSeekg = threadCountSeekg = seekg;
    sem_init(&sem, 0, 1);
    const int NUM_OF_THREADS = count_threads();
    const int NUM_OF_WEIGHTS = count_weights();
    const int outputSize = NUM_OF_WEIGHTS;
    Neuron *neurons = new Neuron[NUM_OF_THREADS];
    // output = new int*[NUM_OF_THREADS];
    // for(int i=0; i<NUM_OF_THREADS; i++){
    //     output[i] = new int[NUM_OF_WEIGHTS];
    // }

    cout << "Number of threads: " << NUM_OF_THREADS << endl;

    //enter inputs
    vector<double> inputs;
    if(process_num == 0){
        double temp = 0.0;
        cout << "Enter inputs:" << endl;
        for(int i=0; i< NUM_OF_THREADS; i++){
            cin >> temp;
            inputs.push_back(temp);
        }
    }
    else
    {
        cout << "reading" << endl;
        //read from pipe
        inputs.resize(numOfInputs);
        int fd = open("pipe", O_RDONLY);
        Neuron nigga;
        read(fd, &nigga.inputs, sizeof(double));
        cout << nigga.inputs << endl;
        // read(fd, &inputs, numOfInputs*sizeof(double));
        close(fd);
    }

    pthread_t *threads = new pthread_t[NUM_OF_THREADS];
    for(int i=0; i<NUM_OF_THREADS; i++){
        neurons[i].thread_number = i;
        neurons[i].inputs = inputs[i];
        neurons[i].output = new double[NUM_OF_WEIGHTS];
        pthread_create(&threads[i], NULL, thread_func, (void*) (&neurons[i]));
    }
    for(int i=0; i<NUM_OF_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    WeightsSeekg = threadCountSeekg;


    mkfifo("pipe", 0666);
    if(fork() == 0){
        if(!finalLayer)
        execlp("./p1", "./p1", to_string(++process_num).c_str(), to_string(WeightsSeekg).c_str(), to_string(outputSize).c_str(),  NULL);
    }
    for (int i = 0; i < NUM_OF_THREADS; i++)
    {
        neurons[i].inputs += neurons[i].output[i];
    }
    int fd = open("pipe", O_WRONLY);
    cout << neurons[0].inputs << endl;
    for (int i = 0; i < NUM_OF_THREADS; i++)
    {
        write(fd, &neurons[i].inputs, sizeof(double));
    }
    // write(fd, &neurons[0].inputs, sizeof(double));
    // write(fd, &neurons[].inputs, sizeof(double));
    close(fd);

    pthread_exit(NULL);

}
