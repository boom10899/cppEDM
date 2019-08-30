
#include "TestCommon.h"

/* simplex signature for reference 
DataFrame<double> Simplex( DataFrame< double > data,
                           std::string pathOut,
                           std::string predictFile,
                           std::string lib,
                           std::string pred,
                           int         E,
                           int         Tp,
                           int         knn,
                           int         tau,
                           std::string columns,
                           std::string target,
                           bool        embedded,
                           bool        verbose ) {
*/

int main () {
    DataFrame< double > cppOutput;

    //----------------------------------------------------------
    // Simplex prediction on block_3sp data : No dynamic embedding
    // Load pyEDM output from:
    //   ./Predict.py -i block_3sp.csv -r x_t -c x_t y_t z_t
    //   -e -l 1 99 -p 100 198 -T 1 -P -o Smplx_embd_block_3sp_pyEDM.csv
    //----------------------------------------------------------

    DataFrame< double > dataFrameIn( "../data/", "Fish1_150a_Normo.csv", true );

    //generate cpp output
    // cppOutput = Simplex ( "../data/", "Fish1_150a_Normo.csv",
    //                       "./data/", "Fish1_150a_Normo_cppEDM.csv",
    //                       "1 300","301 1600", 154, 1, 0, 1,
    //                       "D_21615 D_21628 D_22604 D_35075 D_38999 D_46499 D_49764 D_41309 D_34922 D_46191 D_27071 D_24406 D_25321 D_19632 D_32356 D_46171 D_44441 D_48742 D_44648 D_14683 D_39364 D_18685 D_2752 D_6478 D_49230 D_35787 D_5202 D_45261 D_45270 D_4056 D_5199 D_4065 D_2759 D_5105 D_4000 D_4979 D_18705 D_12291 D_13403 D_37955 D_40016 D_28366 D_38806 D_36693 D_40716 D_18785 D_13366 D_8821 D_17181 D_34265 D_40168 D_39887 D_42633 D_3988 D_23805 D_42365 D_40561 D_29097 D_29569 D_25973 D_4076 D_4096 D_5081 D_35984 D_38187 D_38202 D_33026 D_38679 D_38040 D_13439 D_15353 D_6483 D_18671 D_5120 D_43574 D_40792 D_5083 D_5158 D_4160 D_30065 D_24748 D_44662 D_10461 D_19879 D_28240 D_24988 D_22966 D_30852 D_19979 D_18505 D_14891 D_31763 D_4040 D_6461 D_20176 D_17139 D_52866 D_52861 D_5536 D_13891 D_13955 D_17226 D_23159 D_31159 D_23064 D_28418 D_13643 D_13678 D_28121 D_52158 D_52199 D_25745 D_25752 D_25355 D_5512 D_5519 D_13922 D_6099 D_6108 D_8788 D_18130 D_21025 D_41587 D_41596 D_47934 D_40060 D_42203 D_42576 D_43935 D_33182 D_33224 D_35580 D_38853 D_44217 D_40768 D_9203 D_16672 D_6488 D_9181 D_10318 D_3729 D_47730 D_25115 D_17927 D_12958 D_32946 D_27806 D_33675 D_22485 D_39755 D_31298 D_25250 D_26299 D_Brainsum0", 
    //                       "D_Brainsum0", true, true );

    for (auto& i : dataFrameIn.ColumnNames()) {
        // cppOutput = Simplex ( dataFrameIn,
        //                     "", "",
        //                     "1 300", "301 1600", 1, 1, 0, 1,
        //                     i, i, true, true );
        for(auto& j : dataFrameIn.ColumnNames()) {
            cppOutput = CCM( dataFrameIn,
                     "", "",
                     3, 0, 0, 1, i, j, "1590 1590 1", 1,
                     false, 0, false);
        }
    }
}