// CMSC 341 - Fall 2021 - Project 4
#include "hash.h"
#include <iostream>
#include <random>
#include <vector>
using namespace std;
enum RANDOM {UNIFORM, NORMAL};
class Random {
public:
    Random(int min, int max, RANDOM type=UNIFORM) : m_min(min), m_max(max), m_type(type)
    {
        if (type == NORMAL){
            m_generator = std::mt19937(m_device());
            //the data set will have the mean of 50 and standard deviation of 20
            m_normdist = std::normal_distribution<>(50,20);
        }
        else{
            // Using a fixed seed value generates always the same sequence
            // of pseudorandom numbers, e.g. reproducing scientific experiments
            // here it helps us with testing since the same sequence repeats
            m_generator = std::mt19937(10);// 10 is the fixed seed value
            m_unidist = std::uniform_int_distribution<>(min,max);
        }
    }

    int getRandNum(){
        int result = 0;
        if(m_type == NORMAL){
            //returns a random number in a set with normal distribution
            //we limit random numbers by the min and max values
            result = m_min - 1;
            while(result < m_min || result > m_max)
                result = m_normdist(m_generator);
        }
        else{
            //this will generate a random number between min and max values
            result = m_unidist(m_generator);
        }
        return result;
    }

private:
    int m_min;
    int m_max;
    RANDOM m_type;
    std::random_device m_device;
    std::mt19937 m_generator;
    std::normal_distribution<> m_normdist;//normal distribution
    std::uniform_int_distribution<> m_unidist;//uniform distribution

};

// The hash function used by HashTable class
unsigned int hashCode(const string str);

class Tester{ // Tester class to implement test functions
public:
    Tester(){}
    ~Tester(){}
    void testWithoutCollisions(){
        try{

            cout << "\nTesting hash table with keys that don't collide:" << endl;
            HashTable testTable(MINPRIME,hashCode);
            string fileNames[] = {"These", "Keys", "Should", "Not", "Collide",
                "Hash", "Tables", "Are", "Cool", "!"};
            cout << "\nTesting insert: " << endl;
            for (int i = 0; i < 10; i++){
                if(!testTable.insert(File(fileNames[i], 100000 + i))){
                    throw runtime_error("File could not be inserted");
                }
            }
            cout << "--->Insertion test passed!" << endl;
            cout << "\nTesting hash function: " << endl;
            for (unsigned int i = 0; i < 10; i++){
                unsigned int index =(testTable.m_hash(fileNames[i]) % testTable.m_capacity1);

                if(testTable.m_table1[index].key() != fileNames[i] &&
                   testTable.m_table1[index].diskBlock() != 100000 + i){
                    throw runtime_error("File was not placed at the correct index");
                }
            }
            cout << "--->Hash function test passed!" << endl;
            cout << "\nTesting size of tables: " << endl;
            if((testTable.m_size1 - testTable.m_numDeleted1) +
               (testTable.m_size2 - testTable.m_numDeleted2) != 10){
                throw runtime_error("Not all files were inserted into the system");
            }
            cout << "--->Table size test passed!" << endl;
            cout << "\nTesting file retrieval: " << endl;
            for (unsigned int i = 0; i < 10; i++){
                File foundFile = testTable.getFile(fileNames[i], 100000 + i);
                if(foundFile.key() != fileNames[i] &&
                   foundFile.diskBlock() != 100000 + i){
                    throw runtime_error("getFile() does not work properly");
                }
            }
            cout << "--->Retrieval test passed!" << endl;
            cout << "\nTesting removal: " << endl;
            for (int i = 0; i < 7; i++){
                File file(fileNames[i], 100000 + i);
                if(!testTable.remove(file)){
                    throw runtime_error("File could not be removed");
                }
            }
            for (int i = 0; i < 7; i++){
                File file = testTable.getFile(fileNames[i], 100000 + i);
                if(file.diskBlock()){
                    throw runtime_error("Removed file still in the system");
                }
            }
            cout << "--->File removal test passed!" << endl;
            cout << "\nTesting if rehash starts at appropriate time: " << endl;
            if(testTable.m_newTable == TABLE2)
                throw runtime_error("System started rehashing prematurely");
            File file(fileNames[7], 100000 + 7);
            testTable.remove(file);
            if(testTable.m_newTable != TABLE2)
                throw runtime_error("System did not start rehashing");
            cout << "--->Rehash test passed!" << endl;
            cout << "\n>>>>>Non Collision Test Passed!<<<<<" << endl;
        }catch(const exception &e){
            cout << ">>>>>Non Collision Test Failed!<<<<<" << endl;
            cout << e.what() << endl;
        }
    }

    void testWithCollisions(){
        try{
            cout << "\nTesting hash table with keys that collide: " << endl;
            Random diskBlockGen(DISKMIN,DISKMAX);
            HashTable hashTable(MINPRIME,hashCode);
            int temp = 0;
            int deadDiskBlocks[500] = {0};
            int deadIndex = 0;
            int liveDiskBlocks[500] = {0};
            int liveIndex = 0;
            unsigned int numFiles = 215 ;
            cout << "\nInserting " << numFiles << " files." << endl;
            for (unsigned int i = 0; i < numFiles; i++){
                temp = diskBlockGen.getRandNum();
                if (static_cast<float>(i % 100) / 100.0 >= .8){
                    if(hashTable.insert(File("live.txt", temp))){
                        liveDiskBlocks[liveIndex] = temp;
                        liveIndex++;
                    }else{
                        i--;
                    }
                }else{
                    if(hashTable.insert(File("dead.jpg", temp))){
                        deadDiskBlocks[deadIndex] = temp;
                        deadIndex++;
                    }else{
                        i--;
                    }
                }
            }
            if(hashTable.m_capacity1 != 431)
                throw runtime_error("Table does not have the correct capacity");

            cout << "\nTesting size of the hash tables: " << endl;
            if((hashTable.m_size1 - hashTable.m_numDeleted1) +
               (hashTable.m_size2 - hashTable.m_numDeleted2) != numFiles){
                throw runtime_error("Not all files were inserted into the system");
            }
            cout << "--->Table size test passed!" << endl;

            cout << "\nTesting file removal without resizing: " << endl;
            for (int i = 0; i < 171; i++){
                File file("dead.jpg", deadDiskBlocks[i]);
                if(!hashTable.remove(file)){
                    throw runtime_error("File could not be removed");
                }
            }
            for (int i = 0; i < 171; i++){
                File file = hashTable.getFile("dead.jpg", deadDiskBlocks[i]);
                if(file.diskBlock()){
                    throw runtime_error("Removed file still in the system");
                }
            }
            if(hashTable.m_newTable != TABLE1 || hashTable.m_capacity1 != 431){
                throw runtime_error("Table started rehashing prematurely");
            }
            cout << "--->Removal without resizing passed!" << endl;

            cout << "\nTesting file removal with resizing: " << endl;
            File file("dead.jpg", deadDiskBlocks[171]);
            hashTable.remove(file);
            if(hashTable.m_newTable != TABLE2)
                throw runtime_error("System did not start rehashing");
            if(hashTable.m_capacity2 != 173)
                throw runtime_error("New table does not have correct capacity");
            for (int i = 172; i < deadIndex; i++){
                File file("dead.jpg", deadDiskBlocks[i]);
                if(!hashTable.remove(file)){
                    throw runtime_error("File could not be removed");
                }
            }
            if(hashTable.m_table2)
                throw runtime_error("Table used to transfer was not deallocated");
            cout << "--->Removal with resizing passed!" << endl;

            cout << "\nTesting file retrieval: " << endl;
            for (int i = 0; i < liveIndex; i++) {
                File file("live.txt", liveDiskBlocks[i]);
                if(!(hashTable.getFile("live.txt", liveDiskBlocks[i]) == file)){
                    throw runtime_error("File inserted into the table could not be retrieved");
                }
            }
            cout << "--->File retrieval test passed!" << endl;
            cout << "\n>>>>>Collision Test Passed!<<<<<" << endl;
        }catch(const exception &e){
            cout << ">>>>>Collision Test Failed!<<<<<" << endl;
            cout << e.what() << endl;
        }
    }
    void testEdgeCases(){
        try{
            cout << "\nTesting edge cases:" << endl;
            HashTable tooSmall(5, hashCode);
            if(tooSmall.m_capacity1 != MINPRIME)
                throw runtime_error("Table capacity should be 101");
            HashTable tooBig(100000, hashCode);
            if(tooBig.m_capacity1 != MAXPRIME)
                throw runtime_error("Table capacity should be 99991");
            HashTable notPrime(2048, hashCode);
            if(!notPrime.isPrime(notPrime.m_capacity1) || notPrime.m_capacity1 < 2048)
                throw runtime_error("Table capacity should be a prime number greater than 2048");
            cout << "\n>>>>>Edge cases passed!<<<<<" << endl;
        }catch(const exception &e){
            cout << ">>>>>Test Failed<<<<<" << endl;
            cout << e.what() << endl;
        }
    }
private:

};

int main(){
    // This program presents a sample use of the class HashTable
    // It does not represent any rehashing
    Random diskBlockGen(DISKMIN,DISKMAX);
    int tempDiskBlocks[99991] = {0};
    HashTable aTable(MINPRIME,hashCode);
    int temp = 0;
    int secondIndex = 0;
    for (int i=0;i<99991;i++){
        temp = diskBlockGen.getRandNum();
        if (i%3 == 0){//this saves 17 numbers from the index range [0-49]
            tempDiskBlocks[secondIndex] = temp;
            //cout << temp << " was saved for later use." << endl;
            secondIndex++;
        }
        //cout << "Insertion # " << i << " => " << temp << endl;
        if (i%3 != 0)
            aTable.insert(File("test.txt", temp));
        else
            // these will be deleted
            aTable.insert(File("driver.cpp", temp));
    }

    cout << "Message: dump after 50 insertions in a table with MINPRIME (101) buckets:" << endl;
    aTable.dump();

    for (int i = 0;i<14;i++)
        aTable.remove(File("driver.cpp", tempDiskBlocks[i]));
    cout << "Message: dump after removing 14 buckets," << endl;
    aTable.dump();

    Tester tester;
    tester.testWithoutCollisions();
    tester.testWithCollisions();
    tester.testEdgeCases();
    return 0;
}

unsigned int hashCode(const string str) {
    unsigned int val = 0 ;
    const unsigned int thirtyThree = 33 ;  // magic number from textbook
    for ( long int unsigned i = 0 ; i < str.length(); i++)
        val = val * thirtyThree + str[i] ;
    return val ;
}
