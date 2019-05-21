//---------------------------------------------------------------------
// 1) nThreads CrossMapThread(): Estimate optimal embedding dimension
//    via explicit Simplex() on embedded block, then CCM on all col pairs.
//
// Choose whether to have CrossMapThread() write async to output file
// or store in memory (CrossMap DataFrame) and write at termination
// with #define ASYNC_THREAD_WRITE, i.e. -DASYNC_THREAD_WRITE
//
// NOTE: Presumes first column of Time is not processed
//---------------------------------------------------------------------

// g++ BlockCCM-MT-App.cc -o BlockCCM-MT-App -std=c++11 -I../src -L../lib -lstdc++ -lEDM -lpthread -O3 -DASYNC_THREAD_WRITE -g -DDEBUG

#include <thread>
#include <atomic>

#include "Common.h"
#include "Embed.h"
#include "SyncFile.h" // BUFFER_SIZE

namespace CCMApp {
    // Work Queue : Vector of column names
    typedef std::vector< std::string > WorkQueue;
    
    // atomic counters for all threads
    std::atomic<std::size_t> queue_i(0);
#ifndef ASYNC_THREAD_WRITE
    std::atomic<std::size_t> row_i  (0);
#endif
    std::mutex mutex_;
}

// Forward declaration
void CrossMapThread( CCMApp::WorkQueue           &workQ,
                     DataFrame< double >         &block,
                     DataFrame< double >         &data,
#ifdef ASYNC_THREAD_WRITE
                     std::shared_ptr< SyncFile > &syncFile,
#else
                     DataFrame< double >         &CrossMap,
#endif
                     unsigned                     maxRows,
                     bool                         verbose );

//----------------------------------------------------------------
// 
//----------------------------------------------------------------
int main( int argc, char *argv[] ) {
    
    try {

        std::string fileName      = "../../ZebraFish/Fish_150a_Hypo.csv";
        std::string fileOut       = "EmbedCCM-MT-App_Out.csv";
        unsigned    nThreads      = 40;
        long        maxRows       = 1E15;  // not used if ASYNC_THREAD_WRITE
        bool        verbose       = false;

        if ( argc > 1 ) { fileName = argv[1]; }
        if ( argc > 2 ) { fileOut  = argv[2]; }
        if ( argc > 3 ) { std::stringstream ss( argv[3] ); ss >> nThreads; }
        if ( argc > 4 ) { std::stringstream ss( argv[4] ); ss >> maxRows;  }
        if ( argc > 5 ) { verbose = true; }
        
        // Read data into data frame.  Allocated on heap.
        DataFrame< double > *data = new DataFrame< double >( "", fileName );

        // Column names
        // Remove the first column (Time) from list of column names
        std::vector< std::string >
            colNames( data->ColumnNames().begin() + 1,
                      data->ColumnNames().end() );

        int N_columns = colNames.size();
        
        // Cross Map all columns X all columns + diagonal twice
        long N_CrossMap = N_columns * N_columns + 2 * N_columns;

        if ( maxRows > N_CrossMap ) { maxRows = N_CrossMap; }
        
        std::cout << N_columns << " columns cross-mapped into "
                  << maxRows   << " row pairs..." << std::endl;
        
#ifdef ASYNC_THREAD_WRITE
        // Instantiate synchronized file class
        std::shared_ptr< SyncFile >
            syncFile = std::make_shared< SyncFile >( fileOut );

        // Write header
        syncFile->write( "V1,V2,col_1,col_2,rho,E" );
#else
        // Data frame of CrossMapThread results.
        // JP Allocate on heap
        DataFrame< double > CrossMap( N_CrossMap, 4, "col_1 col_2 rho E" );
#endif
        
        //------------------------------------------------------------------
        // Embed all data to E = 10, columns are added X(t-0) X(t-1)...
        // Allocated on heap by DataFrame copy constructor via MakeBlock
        //------------------------------------------------------------------
        DataFrame< double > *block =
            new DataFrame < double > ( MakeBlock( *data,
                                                   10,   // E
                                                   1,    // tau
                                                   data->ColumnNames(),
                                                   false ) );
        
        // Number of threads
        unsigned maxThreads = std::thread::hardware_concurrency();
        if ( nThreads > maxThreads ) { nThreads = maxThreads; }
        
        // Build work queue : Allocated on heap.
        // Each thread processes cross mapping to all other columns
        CCMApp::WorkQueue *workQ = new CCMApp::WorkQueue( N_columns );

        // Insert column names into work queue
        for ( auto i = 0; i < N_columns; i++ ) {
            workQ->at(i) = colNames[ i ]; // Get variable name
        }

        std::cout << workQ->size() << " elements in work queue. First: "
                  << workQ->at(0)  << " Second: " << workQ->at(1)
                  << " Last: " << workQ->at( workQ->size() - 1 )
                  << " maxThreads " << maxThreads
                  << " nThreads "   << nThreads << std::endl;

        // Thread pool
        std::vector< std::thread > threads;
        
        for ( unsigned i = 0; i < nThreads; ++i ) {
            
            threads.push_back( std::thread( CrossMapThread,
                                            std::ref( *workQ ),
                                            std::ref( *block ),
                                            std::ref( *data  ),
#ifdef ASYNC_THREAD_WRITE
                                            std::ref( syncFile ),
#else
                                            std::ref( CrossMap ),
#endif
                                            maxRows,
                                            verbose ) );
        }

        //-----------------------------------------------------------------
        // join threads
        //-----------------------------------------------------------------
        for ( auto& thrd : threads ) {
            thrd.join();
        }

        //-----------------------------------------------------------------
        // Cleanup
        //-----------------------------------------------------------------
#ifdef ASYNC_THREAD_WRITE
        // Destructor calls syncFile->Close()
#else
        if ( verbose ) {
            std::cout << CrossMap;
        }
        std::cout << "CrossMap Writing " << CrossMap.NRows() << " rows.\n";
        CrossMap.WriteData( "./", fileOut );
#endif

        delete data;
        delete block;
        delete workQ;
        
        std::cout << "Finished." << std::endl;
    }
    catch ( const std::exception& e ) {
 	std::cout << "Exception caught in main:\n";
        std::cout << e.what() << std::endl;        
	return -1;
    }
    catch (...) {
 	std::cout << "Unknown exception caught in main.\n";
	return -1;
    }

    return 0;
}

//----------------------------------------------------------------
// 
//----------------------------------------------------------------
void CrossMapThread( CCMApp::WorkQueue           &workQ,
                     DataFrame< double >         &block,
                     DataFrame< double >         &data,
#ifdef ASYNC_THREAD_WRITE
                     std::shared_ptr< SyncFile > &syncFile,
#else
                     DataFrame< double >         &CrossMap,
#endif
                     unsigned                     maxRows,
                     bool                         verbose )
{
    // atomic_fetch_add(): Adds val to the contained value and returns
    // the value it had immediately before the operation.
    std::size_t workQ_i  = std::atomic_fetch_add( &CCMApp::queue_i,
                                                  std::size_t(1) );
    
#ifdef ASYNC_THREAD_WRITE
    // Object for file output
    SyncFileWrite fileWriter( syncFile );
    
    // JP Should this be thread_local in the global scope?
    //    Doesn't matter since this thread completes with one call...
    size_t thread_buffer_count = 0;
    std::array< std::string, BUFFER_SIZE > thread_buffer;
#else
    std::size_t row_i = std::atomic_fetch_add(&CCMApp::row_i, std::size_t(1));
    
    // Vector for each CCM row to hold V1 V2 rho E
    std::valarray< double > rowVec1( 4 );
    std::valarray< double > rowVec2( 4 );
#endif
    
    // Column names
    std::vector< std::string > colNames = data.ColumnNames();
    
    // library
    int N_rows = block.NRows();
    std::stringstream libraryss;
    libraryss << "1 " << N_rows;
    
    while( workQ_i < workQ.size() ) {
        
        // WorkQueue stores column names
        std::string V1 = workQ[ workQ_i ];

        //----------------------------------------------------------
        // Optimal embedding dimension
        //----------------------------------------------------------
        // Recreate embedded data frame column names X(t-0) X(t-1) for 
        // this V1 for Simplex() to access embedded columns in block
        std::vector< std::string > blockNames( 10 );
        for ( size_t e = 0; e < 10; e++ ) {
            std::stringstream ss;
            ss << V1 << "(t-" << e << ")";
            blockNames[ e ] = ss.str();
        }

        // Container for results
        DataFrame<double> E_rho( 9, 2, "E rho" );
            
        for ( int E = 2; E < 11; E++ ) {
            // blockNames is a vector of embedded column names for Simplex()
            // Select the first E column names for this evaluation
            std::stringstream blockNames_ss;
            for ( int e = 0; e < E; e++ ) {
                blockNames_ss << blockNames[ e ] << " ";
            }

            // Simplex on embedded data
            DataFrame<double> S = Simplex( block,
                                           "",              // pathOut,
                                           "",              // predictFile,
                                           libraryss.str(), // lib,
                                           libraryss.str(), // pred,
                                           E,               // E,
                                           1,               // Tp,
                                           0,               // knn
                                           0,               // tau,
                                           blockNames_ss.str(),
                                           blockNames[0],   // target
                                           true,            // embedded
                                           false );         // verbose
                
            VectorError ve =
                ComputeError( S.VectorColumnName( "Observations" ),
                              S.VectorColumnName( "Predictions"  ) );

            E_rho.WriteRow( E - 2, // poor usage
                            std::valarray<double>({(double) E, ve.rho}));
        }
            
        std::valarray< double > Evec = E_rho.VectorColumnName("E");
        std::valarray< double > rho  = E_rho.VectorColumnName("rho");
        
        auto max_it = std::max_element( begin( rho ), end( rho ) );
        
        size_t max_i = std::distance( begin( rho ), max_it );
        double E_opt = Evec[ max_i ];

        // Could not find decent estimate for E
        if ( E_opt < 2 ) {
            std::lock_guard<std::mutex> lck( CCMApp::mutex_ );
            std::cout << "Thread [" << std::this_thread::get_id()
                      << "] " << V1 << "[" << workQ_i << "]  E_opt=" << E_opt
                      << "   Setting E_opt=6" << std::endl;
            E_opt = 6;
        }
        
        if ( verbose ) {
            std::lock_guard<std::mutex> lck( CCMApp::mutex_ );
            std::cout << "\nThread [" << std::this_thread::get_id()
                      << "] " << V1 << "[" << workQ_i
                      << "]  E_opt=" << E_opt << std::endl;
        }
        
        // lib_size for this E
        int lib_size = N_rows - (int) E_opt;
        std::stringstream lib_sizess;
        lib_sizess << lib_size << " " << lib_size << " 1";

        //--------------------------------------------------------------
        // Cross map V1 against other data columns (V2)
        //--------------------------------------------------------------
        for ( auto j = 1; j < data.NColumns(); j++ ) { // skip time columns
            
            if ( workQ_i + 1 > j ) { continue; }  // Redundant CCM

            // Get variable name
            std::string V2 = colNames[ j ];
            
            if ( verbose ) {
                std::lock_guard<std::mutex> lck( CCMApp::mutex_ );
                std::cout << "  Thread [" << std::this_thread::get_id() << "] "
                          << V1 << "[" << workQ_i << "] : "
                          << V2 << "[" << j       << "]" << std::endl;
            }

            DataFrame< double > CCMD = CCM( data,
                                            "",              // pathOut
                                            "",              // predictFile
                                            E_opt,           // E
                                            0,               // Tp
                                            0,               // knn
                                            1,               // tau
                                            V1,              // colNames
                                            V2,              // targetName
                                            lib_sizess.str(),// libSizes_str
                                            1,               // sample
                                            false,           // random
                                            0,               // seed
                                            false );         // verbose

#ifdef ASYNC_THREAD_WRITE
            std::stringstream ccm_i_j;
            ccm_i_j << V1 << "," << V2 << "," << workQ_i + 1 << "," << j << ","
                    << std::setprecision(4) << CCMD.Row(0)[1]<< "," 
                    << E_opt << '\n'
                    << V2 << "," << V1 << "," << j << "," << workQ_i + 1 << ","
                    << std::setprecision(4) << CCMD.Row(0)[2] << "," << E_opt;
            
            thread_buffer[ thread_buffer_count ] = ccm_i_j.str();
            thread_buffer_count++;

            if ( thread_buffer_count == BUFFER_SIZE ) {
                fileWriter.WriteBuffer( std::ref( thread_buffer ),
                                        thread_buffer_count );
                thread_buffer_count = 0;
                thread_buffer.fill( "" );
            }
        } // for ( auto j = 1; j < data.NColumns(); j++ )
#else
            rowVec1[0] = workQ_i + 1; // can't use V1 = colNames[i]
            rowVec1[1] = j;
            rowVec1[2] = CCMD.Row(0)[1];
            rowVec1[3] = E_opt;
            CrossMap.WriteRow( row_i, rowVec1 );
            
            // increment row counter by one
            row_i = std::atomic_fetch_add( &CCMApp::row_i, std::size_t(1) );

            rowVec2[0] = j; // can't use V1 = colNames[i]
            rowVec2[1] = workQ_i + 1;
            rowVec2[2] = CCMD.Row(0)[2];
            rowVec2[3] = E_opt;
            CrossMap.WriteRow( row_i, rowVec2 );
            
            // increment row counter by one
            row_i = std::atomic_fetch_add( &CCMApp::row_i, std::size_t(1) );
            
            if ( row_i > maxRows ) { break; }
        } // for ( auto j = 1; j < data.NColumns(); j++ )
        if ( row_i > maxRows ) { break; }
#endif
        // increment work queue counter by one
        workQ_i = std::atomic_fetch_add( &CCMApp::queue_i, std::size_t(1) );
        
    } // while( workQ_i < workQ.size() )

    // Flush remaining data
    if ( thread_buffer_count > 0 ) {
        fileWriter.WriteBuffer( std::ref( thread_buffer ),
                                thread_buffer_count );
    }
}
