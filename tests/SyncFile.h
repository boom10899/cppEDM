#ifndef SYNCFILE_H
#define SYNCFILE_H

#include <iostream>
#include <sstream>
#include <mutex>
#include <array>

// Application thread_buffer size
// Keep this small enough to fit on the thread stack (8M)
// If move to heap, then Application writes use ->at() instead of []
// 40 threads with 1000 50 char outputs ~3M
#define BUFFER_SIZE 1000

//----------------------------------------------------------------
// Application level wrapper for ostream with mutex isolation
//----------------------------------------------------------------
class SyncFile {
public:
    // Constructor
    SyncFile( const std::string &fileName ) :
        fileName( fileName ), buffer_count( 0 ) {
        
        // Open file for writing
        fileOutputStream.open( fileName );

        // JP Bad to throw from constructor?
        if ( not fileOutputStream.is_open() ) {
            std::stringstream ErrMsg;
            ErrMsg << "SyncFile(): Failed to open " << fileName << std::endl;
            throw std::runtime_error( ErrMsg.str() );
        }
    }

    ~SyncFile() {
        if ( buffer_count > 0 ) {
            std::lock_guard< std::mutex > lock( syncFileMutex );
            flushBuffer();
        }
        Close();
    }
    
    //------------------------------------------------------------
    void Close() {
        if ( fileOutputStream.is_open() ) {
            fileOutputStream.close();
        }
    }
    
    //------------------------------------------------------------
    // Write data string to file
    //------------------------------------------------------------
    void write( const std::string &data ) {
        // Ensure that only one thread can execute at a time
        std::lock_guard< std::mutex > lock( syncFileMutex );
        
        fileOutputStream << data << std::endl; // implicitly flush
    }
    
    //------------------------------------------------------------
    // Write array<string> buffer to file
    //------------------------------------------------------------
    void writeBuffer( const std::array< std::string, BUFFER_SIZE > &data,
                      size_t count ) {
        std::lock_guard< std::mutex > lock( syncFileMutex );

        for ( size_t i = 0; i < count; i++ ) {
            fileOutputStream << data[ i ] << '\n';
        }
        fileOutputStream << std::flush;
    }
    
    //------------------------------------------------------------
    // Insert data string into SyncFile::buffer
    // Write to file when buffer is full
    //------------------------------------------------------------
    void writeBuffer( const std::string &data ) {
        std::lock_guard< std::mutex > lock( syncFileMutex );

        buffer[ buffer_count ] = data;
        buffer_count++;
        
        // If the buffer is full, write and clear
        if ( buffer_count == BUFFER_SIZE - 1 ) {
            flushBuffer();
        }
    }

    //------------------------------------------------------------
    void flushBuffer() {
        for ( size_t i = 0; i < buffer_count; i++ ) {
            fileOutputStream << buffer[ i ] << '\n';
        }
        fileOutputStream << std::flush;
        
        // Clear the buffer and reset count
        buffer.fill( "" );
        buffer_count = 0;
    }
    
public:
    size_t buffer_count;
    std::array< std::string, BUFFER_SIZE > buffer;
    
private:
    std::ofstream fileOutputStream;
    std::string   fileName;
    std::mutex    syncFileMutex;
};

//----------------------------------------------------------------
// Thread level interface to SyncFile ostream
//----------------------------------------------------------------
class SyncFileWrite {
public:
    SyncFileWrite( std::shared_ptr< SyncFile > sf ) : syncFile( sf ) {}
    
    void Write( std::string data ) {     
        syncFile->write( data );
    }
    
    void WriteBuffer( std::string data ) {
        syncFile->writeBuffer( data );
    }
    
    void WriteBuffer( const std::array< std::string, BUFFER_SIZE > &data,
                      size_t count ) {
        syncFile->writeBuffer( data, count );
    }
    
private:
    std::shared_ptr< SyncFile > syncFile;
};
#endif
