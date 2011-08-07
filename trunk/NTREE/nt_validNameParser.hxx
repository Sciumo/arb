// =============================================================== //
//                                                                 //
//   File      : nt_validNameParser.hxx                            //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Lothar Richter in November 2002                      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef NT_VALIDNAMEPARSER_HXX
#define NT_VALIDNAMEPARSER_HXX

#ifndef __LIST__
#include <list>
#endif
#ifndef __STRING__
#include <string>
#endif
#ifndef __VECTOR__
#include <vector>
#endif

namespace validNames {

    typedef std::list<std::string> LineSet;
    typedef LineSet* LineSetPtr;

    typedef std::vector<std::string> TokL;
    typedef TokL *TokLPtr;

    typedef enum {
        VALGEN, HETGEN, HOMGEN, RENGEN, CORGEN,
        VALSPEC, HETSPEC, HOMSPEC, RENSPEC, CORSPEC,
        NOTYPE, VAL, HET, HOM, REN, COR
    } DESCT;

    class Desco {
    private:
        DESCT type;
        bool isCorrected;
        std::string firstgen;
        std::string firstspec;
        std::string firstsub;
        std::string secondgen;
        std::string secondspec;
        std::string secondsub;

    public:
        Desco(DESCT       type_,  bool isCorrected_,
              std::string firstgen_, std::string firstspec_, std::string firstsub_,
              std::string secondgen_, std::string secondspec_, std::string secondsub_)
            : type(type_)
            , isCorrected(isCorrected_)
            , firstgen(firstgen_)
            , firstspec(firstspec_)
            , firstsub(firstsub_)
            , secondgen(secondgen_)
            , secondspec(secondspec_)
            , secondsub(secondsub_)
        {}

        std::string getFirstName();
        std::string getSecondName();
        DESCT getType() const { return type; }
    };




    LineSet* readFromFile(const char* infile, LineSet* listOfLines);

    TokLPtr tokenize(const std::string& description, TokLPtr tokenLP);

    Desco determineType(const std::string& descriptionString);

    bool isUpperCase(const std::string& input);
}; // end namespace

#else
#error nt_validNameParser.hxx included twice
#endif // NT_VALIDNAMEPARSER_HXX
