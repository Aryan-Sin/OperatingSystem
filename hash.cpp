// CMSC 341 - Fall 2021 - Project 4
#include "hash.h"
#include "math.h"
HashTable::HashTable(unsigned size, hash_fn hash){
    m_hash = hash;
    m_newTable = TABLE1;
    //checking whether the size given is within range and prime
    if(size <= MINPRIME)
        m_capacity1 = MINPRIME;
    else if(size >= MAXPRIME)
        m_capacity1 = MAXPRIME;
    else{
        //if the size is within range but not a prime number, get the next prime
        m_capacity1 = (isPrime(size)) ? size : findNextPrime(size);
    }
    m_size1 = 0;
    m_numDeleted1 = 0;
    m_table1 = new File[m_capacity1];
    //initializing table1 files to empty
    for(unsigned int i = 0; i < m_capacity1; i++)
        m_table1[i] = EMPTY;
    
    
    //in this implementation table2 will only be used for rehashing, after which
    //m_table1 will point to table2 and m_table2 will be set to nullptr. Table1
    //will be deleted.
    m_capacity2 = 0;
    m_size2 = 0;
    m_numDeleted2 = 0;
    m_table2 = nullptr;
}

HashTable::~HashTable(){
    delete [] m_table1;
    if(m_table2)
        delete [] m_table2;
}

File HashTable::getFile(string name, unsigned int diskBlock){
    //check if the file being requested is a valid file
    if(diskBlock < DISKMIN || diskBlock > DISKMAX || !name.size())
        return EMPTY;
    //checking whether the table is being transferred currently
    if(m_table2){
        bool checkingNew = true, checkingOld = true;
        unsigned int newIndex, oldIndex;
        unsigned int newHash = m_hash(name) % m_capacity2;
        unsigned int oldHash = m_hash(name) % m_capacity1;
        for(unsigned int i = 0; i < m_capacity1; i++){
            if(checkingNew){
                //finding the index of the file using the name as the key
                newIndex = (newHash + (i * i)) % m_capacity2;
                //checking if the file at the index location is the file we are
                //looking for
                if(m_table2[newIndex].diskBlock() == diskBlock &&
                   m_table2[newIndex].key() == name){
                    return m_table2[newIndex];
                }else if(!m_table2[newIndex].key().size()){
                    //if we encounter an empty cell the file is not in our table
                    //so we stop searching the current table
                    checkingNew = false;
                }
            }
            if(checkingOld){
                //we perform the same search in the old table
                oldIndex = (oldHash + (i * i)) % m_capacity1;
                if(m_table1[oldIndex].diskBlock() == diskBlock &&
                   m_table1[oldIndex].key() == name){
                    return m_table1[oldIndex];
                }else if(!m_table1[oldIndex].key().size()){
                    checkingOld = false;
                }
            }else if(!checkingNew){
                //check if both the current table and the old table found empty
                //buckets, in which case the file does not exist in our system
                return EMPTY;
            }
        }
    }else{
        //if we are not rehashing the table, we check only table1
        unsigned int index;
        unsigned int hash = m_hash(name) % m_capacity1;
        for(unsigned int i = 0; i <= m_size1; i++){
            index = (hash + (i * i)) % m_capacity1;
            if(m_table1[index].diskBlock() == diskBlock &&
               m_table1[index].key() == name)
                return m_table1[index];
            else if(!m_table1[index].key().size())
                return EMPTY;
        }
    }
    return EMPTY;
}

bool HashTable::insert(File file){
    //check whether we are rehashing the system
    if(m_table2){
        unsigned int index;
        unsigned int hash = m_hash(file.key()) % m_capacity2;
        //insert file into the new table
        for (unsigned int i = 0; i <= m_capacity2; i++) {
            index = (hash + (i * i)) % m_capacity2;
            //check whether the location is occupied with a live file
            if(!m_table2[index].diskBlock()){
                //check if the location was deleted, in which case decrement the
                //numDeleted counter
                if(m_table2[index].key().size()){
                    m_table2[index] = file;
                    m_numDeleted2--;
                }else{
                    //if the location was empty, increment the size
                    m_table2[index] = file;
                    m_size2++;
                }
                //transfer 25% of the old table into the new table
                resizeTable();
                return true;
            }else{
                //check whether the file already exists in the system, in which
                //case return false since our system can not have dublicate
                //Disk Block numbers
                if(m_table2[index].diskBlock() == file.diskBlock()){
                    resizeTable();
                    return false;
                }
            }
        }
    }else{
        //if we are not rehashing, check if the table is full
        if(m_size1 == m_capacity1 && !m_numDeleted1){
            cout << "Max file capacity reached, no new files can be inserted until an existing file is removed." << endl;
            return false;
        }
        unsigned int hash = m_hash(file.key()) % m_capacity1;
        unsigned int index;
        for(unsigned int i = 0; i <= m_capacity1; i++){
            index = (hash + (i * i)) % m_capacity1;
            //check if the location is deleted or empty
            if(!m_table1[index].diskBlock()){
                if(m_table1[index].key().size()){
                    m_table1[index] = file;
                    m_numDeleted1--;
                }else{
                    m_table1[index] = file;
                    m_size1++;
                }
                //check whether the insertion caused the load factor to be
                //greater than critical load factor, in which case initiate
                //rehashing of the system
                if(lambda(m_newTable) >= CRITICAL_LOAD)
                    resizeTable();
                return true;
            }else
                //if the file already exists in the system return false
                if(m_table1[index].diskBlock() == file.diskBlock())
                    return false;
        }
    }
    return false;
}

void HashTable::resizeTable(){
    if(m_table2){
        //variables to determine where to start and stop the transfer
        unsigned int transferIndex = (m_size2) ? m_table1[0].diskBlock() - DISKMIN : 0;
        unsigned int numChecked = ((m_size2) ? m_table1[1].diskBlock() - DISKMIN : 0);
        unsigned int endPoint = (numChecked + 1) + ((m_size1) / 4);

        //traversing the old table (which will always be m_table1) and
        //transferring 25% of the live files.
        while(numChecked < endPoint && transferIndex < m_capacity1){
            //check if the current location in the old table contains a live
            //file
            if(m_table1[transferIndex].diskBlock()){
                //if a live file is found, insert the file into the new table
                unsigned int hash = (m_hash(m_table1[transferIndex].key()) %
                                     m_capacity2);
                unsigned int newIndex;
                for (unsigned int i = 0; i <= m_size2; i++) {
                    newIndex = (hash + (i * i)) % m_capacity2;
                    if(!m_table2[newIndex].diskBlock()){
                        if(m_table2[newIndex].key().size()){
                            m_table2[newIndex] = m_table1[transferIndex];
                            m_numDeleted2--;
                            break;
                        }else{
                            m_table2[newIndex] = m_table1[transferIndex];
                            m_size2++;
                            break;
                        }
                    }else if(m_table2[newIndex].diskBlock() == m_table1[transferIndex].diskBlock())
                        break;
                }
                //set the file to deleted in the old table and increment counter
                //for deleted and number of non-empty files checked
                m_table1[transferIndex] = DELETED;
                m_numDeleted1++;
                numChecked++;
            }else if(m_table1[transferIndex].key().size())
                //if the file is deleted, don't insert the file into the new
                //table but still increment the counter for number of non-empty
                //files checked
                numChecked++;
            transferIndex++;
        }
        //checking whether we have inserted all live files into the new table
        if(transferIndex == m_capacity1 || numChecked == m_size1){
            //if all of the live files have been transferred into table2,
            //deallocate the memory for the old table (table1) and set table1
            //pointer to the new table. Finally set the pointer of table2 to
            //nullptr
            delete [] m_table1;
            m_capacity1 = m_capacity2;
            m_size1 = m_size2;
            m_numDeleted1 = m_numDeleted2;
            m_table1 = m_table2;
            m_capacity2 = 0;
            m_size2 = 0;
            m_numDeleted2 = 0;
            m_table2 = nullptr;
            //set table1 as the new table
            m_newTable = TABLE1;
        }
        else{
            //store relevant values about the transfer in the first and second
            //files of the old table, which will always be empty since the first
            //quarter of the old table will be transferred before these values
            //are needed
            m_table1[0] = File(m_table1[0].key(), transferIndex + DISKMIN);
            m_table1[1] = File(m_table1[1].key(), numChecked + DISKMIN);
        }
    }else{
        //checking if it is possible to resize the array
        if(m_capacity1 == MAXPRIME && deletedRatio(TABLE1) <
           CRITICAL_DELETED_RATIO)
            //do nothing because table is already at max size
            return;
        unsigned int newTableCapacity = findNextPrime((m_size1 -
                                                       m_numDeleted1) * 4);
        //allocating memory for a new table
        m_table2 = new File[newTableCapacity];
        //initializing member variables for the new table
        m_capacity2 = newTableCapacity;
        m_size2 = 0;
        m_numDeleted2 = 0;
        //setting newTable to m_table2
        m_newTable = TABLE2;
        for (unsigned int i = 0; i < m_capacity2; i++)
            //initilize all elements in the array to empty files
            m_table2[i] = EMPTY;
        //calling itself to transfer 25% of the files into the
        //new table
        resizeTable();
        
    }
}

bool HashTable::remove(File file){
    if(m_table2){
        //bool variables to keep track of which tables still need to be checked
        bool checkingCurr = true, checkingOld = true;
        unsigned int index, oldIndex;
        unsigned int newHash = (m_hash(file.key()) % m_capacity2);
        unsigned int oldHash = (m_hash(file.key()) % m_capacity1);
        for(unsigned int i = 0; i <= m_size1; i++){
            if(checkingCurr){
                //check the new table for the file
                index = (newHash + (i * i)) % m_capacity2;
                if((m_table2[index].diskBlock() == file.diskBlock()) && (m_table2[index].key() == file.key())){
                    m_table2[index] = DELETED;
                    m_numDeleted2++;
                    //if file is found, move 25% of the files into the new table
                    //before returning true
                    resizeTable();
                    return true;
                }else if(!m_table2[index].key().size()){
                    //if the location being checked is empty, the file is not in
                    //the new table so we can stop checking the new table
                    checkingCurr = false;
                }
            }
            if(checkingOld){
                oldIndex = (oldHash + (i * i)) % m_capacity1;
                if((m_table1[oldIndex].diskBlock() == file.diskBlock()) && (m_table1[oldIndex].key() == file.key())){
                    m_table1[oldIndex] = DELETED;
                    m_numDeleted1++;
                    resizeTable();
                    return true;
                }else if(!m_table1[oldIndex].key().size()){
                    checkingOld = false;
                }
            }else if(!checkingCurr){
                //if we are no longer checking eather of the tables because they
                //have both resulted in an EMPTY file being found, then the file
                //is not in our system. Transfer 25% of the files into the new
                //table before returning false
                resizeTable();
                return false;
            }
        }
    }else{
        //if system is not rehashing, then find the file in table1 and mark it
        //as deleted
        unsigned int hash = (m_hash(file.key()) % m_capacity1);
        unsigned int index;
        for(unsigned int i = 0; i <= m_size1; i++){
            index = (hash + (i * i)) % m_capacity1;
            if(m_table1[index].diskBlock()){
                if((m_table1[index].diskBlock() == file.diskBlock()) &&
                   (m_table1[index].key() == file.key())){
                    m_table1[index] = DELETED;
                    m_numDeleted1++;
                    //check whether the deleted ratio has reached the critical ratio
                    if(deletedRatio(m_newTable) >= CRITICAL_DELETED_RATIO)
                        resizeTable();
                    return true;}
            }else if(m_table1[index].key() != DELETEDKEY)
                return false;
        }
    }
    return false;
}

float HashTable::lambda(TABLENAME tablename) const{
    //check which table needs to be checked
    if(tablename == TABLE1)
        //cast the unsigned integers to floats and return the result
        return (static_cast<float>(m_size1 - m_numDeleted1) /
                static_cast<float>(m_capacity1));
    else
        return (static_cast<float>(m_size2 - m_numDeleted2) /
                static_cast<float>(m_capacity2));
}

float HashTable::deletedRatio(TABLENAME tableName) const{
    //check which table needs to be checked
    if(tableName == TABLE1)
        //cast the unsigned integers to floats and return the result
        return (static_cast<float>(m_numDeleted1) /
                static_cast<float>(m_size1));
    else
        return (static_cast<float>(m_numDeleted2) /
                static_cast<float>(m_size2));
}

void HashTable::dump() const{
    cout << endl << "Dump for table 1: " << endl;
    if (m_table1 != nullptr)
        for (unsigned int i = 0; i < m_capacity1; i++)
            cout << "[" << i << "] : " << m_table1[i] << endl;
    cout << "Dump for table 2: " << endl;
    if (m_table2 != nullptr)
        for (unsigned int i = 0; i < m_capacity2; i++)
            cout << "[" << i << "] : " << m_table2[i] << endl;
}

bool HashTable::isPrime(int number){
    bool result = true;
    for (int i = 2; i <= number / 2; ++i){
        if (number % i == 0){
            result = false;
            break;
        }
    }
    return result;
}

int HashTable::findNextPrime(int current){
    //we always stay within the range [MINPRIME-MAXPRIME]
    //the smallest prime starts at MINPRIME
    if (current < MINPRIME) current = MINPRIME-1;
    for (int i = current; i < MAXPRIME; i++){
        for (int j = 2; j * j <= i; j++){
            if (i % j == 0)
                break;
            else if (j + 1 > sqrt(i) && i != current)
                return i;
        }
    }
    //if a user tries to go over MAXPRIME
    return MAXPRIME;
}
