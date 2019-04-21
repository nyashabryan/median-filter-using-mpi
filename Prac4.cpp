//==============================================================================
// Copyright (C) John-Philip Taylor
// tyljoh010@myuct.ac.za
//
// This file is part of the EEE4084F Course
//
// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>
//
// This is an adaptition of The "Hello World" example avaiable from
// https://en.wikipedia.org/wiki/Message_Passing_Interface#Example_program
//==============================================================================


/** \mainpage Prac4 Main Page
 *
 * \section intro_sec Introduction
 *
 * The purpose of Prac4 is to learn some basics of MPI coding.
 *
 * Look under the Files tab above to see documentation for particular files
 * in this project that have Doxygen comments.
 */



//---------- STUDENT NUMBERS --------------------------------------------------
//
// KTMNYA001 MZRTAD001
//
//-----------------------------------------------------------------------------

/* Note that Doxygen comments are used in this file. */
/** \file Prac4
 *  Prac4 - MPI Main Module
 *  The purpose of this prac is to get a basic introduction to using
 *  the MPI libraries for prallel or cluster-based programming.
 */

// Includes needed for the program
#include "Prac4.h"
#include <iostream>

using namespace std;

/** This is the master node function, describing the operations
    that the master will be doing */
void Master()
{
    //! <h3>Local vars</h3>
    // The above outputs a heading to doxygen function entry
    int j;              //! j: Loop counter
    MPI_Status stat;    //! stat: Status of the MPI application

    // Read the input image
    if (!Input.Read("Data/small.jpg"))
    {
        printf("Cannot read image\n");
        return;
    }

    // Print the number of processes available
    printf("0: We have %d processes\n", numprocs);

    // Declare the header data to be sent to the processes.
    int header[numprocs][HEADER_SIZE];

    // Start measuring the time span
    tic();

    /**
     * Build the message headers
     */
    int share = Input.Height / (numprocs - 1) ;


    /**
     * Send the message headers to all the different processes
     */
    for (j = 1; j < numprocs; j++)
    {
        header[j - 1][0] = share;
        if (j == numprocs -1){
            header[j-1][0] = Input.Height - ((j-1) * share);
        }

        header[j-1][1] = Input.Width;

        header[j -1][2] = Input.Components;

        MPI_Send(header[j -1], HEADER_SIZE, MPI_INT, j, TAG, MPI_COMM_WORLD);
    }

    /**
     * Send the data to the different processes
     */

    int row_buffer[Input.Width * Input.Components];
    // Loop through the processes in order.
    int current_row = 0;
    for(j = 1; j < numprocs; j++){

        // loop through each of the rows of the 
        for(int i = 0; i < header[j-1][0]; i++){

            // Assign values to the row buffer
            for(int s = 0; s < Input.Width * Input.Components; s++){
                row_buffer[s] = Input.Rows[current_row][s];
            }
            current_row++;

            // build a 
            MPI_Send(row_buffer, (Input.Width * Input.Components), MPI_INT, j, TAG, MPI_COMM_WORLD);
        }
    }

    // Allocated RAM for the output image
    if (!Output.Allocate(Input.Width, Input.Height, Input.Components))
        return;

    /**
     * Receive the buffer data from the different processes and append it to the 
     * Output JPEG object. 
     */

    current_row = 0;
    for (j = 1; j < numprocs; j++)
    {
        for(int d = 0; d < header[j-1][0]; d++){
            // Receive the processed data
            MPI_Recv(row_buffer, Input.Width * Input.Components, MPI_INT, j, TAG, MPI_COMM_WORLD, &stat);

            // send received buffer to output
            for (int k = 0; k < Input.Width * Input.Components; k++)
            {
                Output.Rows[current_row][k] = row_buffer[k];
            }
            current_row++;
        }
    }

    printf("Time = %lg ms\n", (double)toc() / 1e-3);

    // Write the output image
    if (!Output.Write("Data/Output.jpg"))
    {
        printf("Cannot write image\n");
        return;
    }
    //! <h3>Output</h3> The file Output.jpg will be created on success to save
    //! the processed output.
}
//------------------------------------------------------------------------------

/** This is the Slave function, the workers of this MPI application. */
void Slave(int ID)
{

    char idstr[32];

    int header[HEADER_SIZE];

    MPI_Status stat;

    // receive from rank 0 (master):
    // This is a blocking receive, which is typical for slaves.
    MPI_Recv(header, HEADER_SIZE, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);

    // Inform cmd that header has been received.

    cout << "Process " << ID << " has received header" << endl;
    cout << header[0] << " " << header[1] << " " << header[2] << endl;

    // Allocate space for the incoming data.
    JPEG data;
    if(!data.Allocate(header[1], header[0], header[2]))
    {
        cout << "Process number " << ID << " has failed to allocate memory" << endl;
        exit(1);
    }

    /**
     * Receive the data from master
     */
    int current_row[data.Components * data.Width];
    for (int i = 0; i < data.Height; i++){
        MPI_Recv(current_row, data.Components * data.Width, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
        for(int j= 0; j < data.Components * data.Width; j++){
            data.Rows[i][j] = current_row[j];
        }
    }

    cout << "Data allocated" << endl;
    // Allocate space for the output data.
    JPEG result;
    if(!result.Allocate(data.Width, data.Height, data.Components))
    {
        cout << "Process number " << ID << " has failed to allocate memory" << endl;
        exit(1);
    }

    cout << "Process " << ID << " is starting to calculate median "<< endl;


    /**
     * Process the median filter for the data
     */

    int current;
    int arrayOfValues[81];
    int median;

    for(int k =0; k < data.Components; k++){
        for(int i = 0; i < data.Height; i++){
            for(int j=k; j < data.Width * data.Components; j = j + 3){
                
                current = 0;
                for(int s = i - 4; s< i + 5; s++){
                    for(int t = j - 4 * data.Components; t < j + 4 * data.Components + 1; t = t+3){
                        if(s < 0 || t < 0 || s >= data.Height || t >= data.Width * data.Components){
                            arrayOfValues[current] = data.Rows[i][j];
                        }else {
                            arrayOfValues[current] = data.Rows[s][t];
                        }
                        current++;
                    }
                }
                quickSort(arrayOfValues, 0, 80);
                median = arrayOfValues[41];
                for(int s = i - 4; s< i + 5; s++){
                    for(int t = j - 4 * result.Components; t < j + 4 * result.Components + 1; t = t+3){
                        if(!(s < 0 || t < 0 || s >= data.Height || t >= data.Width * data.Components)){
                            result.Rows[s][t] = median;
                        }
                    }
                }
            }
        }
    }

    cout << "Process: "<< ID << " has finished calculation" << endl;

    /**
     * Send back data to the master 
     */

    // Loop through the rows 
    for(int r = 0; r < data.Height; r++){
        
        for(int s = 0; s < data.Width * data.Components; s++){
            current_row[s] = result.Rows[r][s];
        }
        MPI_Send(current_row, data.Width * data.Components, MPI_INT, 0, TAG, MPI_COMM_WORLD);
    }

    cout << "Process " << ID << " finished" << endl;
}


//------------------------------------------------------------------------------

/** This is the entry point to the program. */
int main(int argc, char **argv)
{
    int myid;

    // MPI programs start with MPI_Init
    MPI_Init(&argc, &argv);

    // find out how big the world is
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    // and this processes' rank is
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);

    // At this point, all programs are running equivalently, the rank
    // distinguishes the roles of the programs, with
    // rank 0 often used as the "master".
    if (myid == 0)
        Master();
    else
        Slave(myid);

    // MPI programs end with MPI_Finalize
    MPI_Finalize();
    return 0;
}
//------------------------------------------------------------------------------

/** The quick sort implementations which is used the calculation of the median
 * filter.
    arr[] --> Array to be sorted, 
    low  --> Starting index, 
    high  --> Ending index */
void quickSort(int arr[], int low, int high)
{ 
    if (low < high) 
    { 
        /* pi is partitioning index, arr[p] is now 
           at right place */
        int pi = partition(arr, low, high); 
  
        // Separately sort elements before 
        // partition and after partition 
        quickSort(arr, low, pi - 1); 
        quickSort(arr, pi + 1, high); 
    } 
}


  
/* This function takes last element as pivot, places 
   the pivot element at its correct position in sorted 
    array, and places all smaller (smaller than pivot) 
   to left of pivot and all greater elements to right 
   of pivot */
int partition (int arr[], int low, int high)
{ 
    int pivot = arr[high];    // pivot 
    int i = (low - 1);  // Index of smaller element 
  
    for (int j = low; j <= high- 1; j++) 
    { 
        // If current element is smaller than or 
        // equal to pivot 
        if (arr[j] <= pivot) 
        { 
            i++;    // increment index of smaller element 
            swap(&arr[i], &arr[j]); 
        } 
    } 
    swap(&arr[i + 1], &arr[high]); 
    return (i + 1); 
}

/**
 * The swap function to swap two integers using their pointers.
 */
void swap(int* a, int* b) 
{ 
    int t = *a; 
    *a = *b; 
    *b = t; 
} 