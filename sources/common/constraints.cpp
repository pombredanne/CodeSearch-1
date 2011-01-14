//@+leo-ver=5-thin
//@+node:gcross.20101231214817.2240: * @thin constraints.cpp
//@@language cplusplus

//@+<< Includes >>
//@+node:gcross.20101231214817.2241: ** << Includes >>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <map>

#include "constraints.hpp"
#include "constraints/standard_form.hpp"
#include "operator_space.hpp"
#include "utilities.hpp"
//@-<< Includes >>

namespace CodeSearch {

//@+<< Usings >>
//@+node:gcross.20101231214817.2242: ** << Usings >>
using namespace Gecode;
using namespace boost::assign;
//@-<< Usings >>

//@+<< Function declaration >>
//@+node:gcross.20110112003748.1557: ** << Function declaration >>
StandardFormTies postColumnOrderingConstraints(
      OperatorSpace& space
    , const StandardFormParameters& parameters
    , StandardFormTies initial_ties=no_standard_form_ties
);

StandardFormTies postRowOrderingConstraints(
      function<BoolVarArgs
        ( OperatorSpace& space
        , BoolMatrix region
        , BoolVarArgs initial_ties
        )
      > postConstraintForRegion
    , OperatorSpace& space
    , const StandardFormParameters& parameters
    , StandardFormTies initial_ties=no_standard_form_ties
);

BoolVarArgs postWeightRowOrderingConstraintOnRegion(
      OperatorSpace& space
    , BoolMatrix region
    , BoolVarArgs initial_ties=BoolVarArgs()
);
//@-<< Function declaration >>

//@+others
//@+node:gcross.20110112003748.1550: ** Values
const static map<
     Constraint
    ,function<BoolVarArgs
        ( OperatorSpace& space
        , BoolMatrix region
        , BoolVarArgs initial_ties
        )
      >
    > constraint_posters = map_list_of(WeightRowOrderingConstraint,postWeightRowOrderingConstraintOnRegion);
//@+node:gcross.20110112003748.1547: ** Functions
//@+node:gcross.20101231214817.2245: *3* createConstraintedSpace
auto_ptr<OperatorSpace> createConstrainedSpace(
      const unsigned int number_of_qubits
    , const unsigned int number_of_operators
    , const set<Constraint>& constraints
    , const StandardFormParameters& parameters
) {
    auto_ptr<OperatorSpace> space(new OperatorSpace(number_of_qubits,number_of_operators));
    StandardFormTies current_ties = no_standard_form_ties;
    BOOST_FOREACH(const Constraint constraint, constraints) {
        switch(constraint) {
            case StandardForm:
                postStandardFormConstraint(*space,parameters);
                break;
            case ColumnOrdering:
                current_ties = postColumnOrderingConstraints(*space,parameters);
                break;
            default:
                current_ties = postRowOrderingConstraints(
                     constraint_posters.find(constraint)->second
                    ,*space
                    ,parameters
                    ,current_ties
                );
        }
    }
    return space;
}
//@+node:gcross.20110112003748.1559: *3* postColumnOrderingConstraintOnRegion
BoolVarArgs postColumnOrderingConstraintOnRegion(OperatorSpace& space, BoolMatrix variables, BoolVarArgs initial_ties) {
    return postOrderingConstraint(space,variables,initial_ties);
}
//@+node:gcross.20110112003748.1552: *3* postColumnOrderingConstraints
StandardFormTies postColumnOrderingConstraints(
      OperatorSpace& space
    , const StandardFormParameters& parameters
    , StandardFormTies initial_ties
) {
    const unsigned int
          number_of_qubits = space.number_of_qubits
        , number_of_operators = space.number_of_operators
        , x_bit_diagonal_size = parameters.x_bit_diagonal_size
        , z_bit_diagonal_size = parameters.z_bit_diagonal_size
        ;
    BoolMatrix X_matrix = space.getXMatrix()
             , Z_matrix = space.getZMatrix()
             ;
    postOrderingConstraint(
         space
        ,X_matrix.slice(
             x_bit_diagonal_size
            ,number_of_qubits
            ,0
            ,x_bit_diagonal_size
         )
        ,postOrderingConstraint(
             space
            ,Z_matrix.slice(
                 x_bit_diagonal_size
                ,number_of_qubits
                ,0
                ,number_of_operators
             )
         )
    );
    return make_pair(
         BoolVarArgs()
        ,postOrderingConstraint(
             space
            ,Z_matrix.slice(
                 z_bit_diagonal_size
                ,x_bit_diagonal_size
                ,x_bit_diagonal_size
                ,number_of_operators
             )
            ,postOrderingConstraint(
                 space
                ,Z_matrix.slice(
                     z_bit_diagonal_size
                    ,x_bit_diagonal_size
                    ,0
                    ,z_bit_diagonal_size
                 )
             )
         )
    );
}
//@+node:gcross.20110112003748.1556: *3* postOrderingConstraint
BoolVarArgs postOrderingConstraint(OperatorSpace& space, IntMatrix variables, BoolVarArgs initial_ties) {
    if(variables.width() <= 1 || variables.height() <= 0) return initial_ties;

    const unsigned int number_of_cols = variables.width()
                     , number_of_rows = variables.height()
                     ;

    BoolVarArgs ties(space,(number_of_rows+1)*(number_of_cols-1),0,1);
    BoolMatrix ties_matrix(ties,number_of_cols-1,number_of_rows+1);

    if(initial_ties.size() > 0) {
        assert(initial_ties.size() == variables.width()-1);
        BOOST_FOREACH(const unsigned int i, irange(0u,number_of_cols-1)) {
            rel(space,ties_matrix(i,0),IRT_EQ,initial_ties[i]);
        }
    } else {
        BOOST_FOREACH(const unsigned int i, irange(0u,number_of_cols-1)) {
            rel(space,ties_matrix(i,0),IRT_EQ,1);
        }
    }

    BOOST_FOREACH(const unsigned int row, irange(0u,number_of_rows)) {
        BOOST_FOREACH(const unsigned int col, irange(0u,number_of_cols-1)) {
            ties_matrix(col,row+1) = expr(space,ties_matrix(col,row) && variables(col,row) == variables(col+1,row));
            rel(space,ties_matrix(col,row) >> (variables(col,row) >= variables(col+1,row)));
        }
    }

    return ties_matrix.row(number_of_rows);
}

BoolVarArgs postOrderingConstraint(OperatorSpace& space, BoolMatrix variables, BoolVarArgs initial_ties) {
    return postOrderingConstraint(space,channelMatrix(space,variables),initial_ties);
}
//@+node:gcross.20110112003748.1549: *3* postRowOrderingConstraints
StandardFormTies postRowOrderingConstraints(
      function<BoolVarArgs
        ( OperatorSpace& space
        , BoolMatrix region
        , BoolVarArgs initial_ties
        )
      > postConstraintForRegion
    , OperatorSpace& space
    , const StandardFormParameters& parameters
    , StandardFormTies initial_ties
) {
    const unsigned int
          number_of_qubits = space.number_of_qubits
        , number_of_operators = space.number_of_operators
        , x_bit_diagonal_size = parameters.x_bit_diagonal_size
        , z_bit_diagonal_size = parameters.z_bit_diagonal_size
        ;
    BoolMatrix X_matrix = space.getXMatrix()
             , Z_matrix = space.getZMatrix()
             ;
    return make_pair(
          postConstraintForRegion(
              space
            , Z_matrix.slice(
                  z_bit_diagonal_size
                , number_of_qubits
                , x_bit_diagonal_size
                , number_of_operators
              )
            , postConstraintForRegion(
                  space
                , Z_matrix.slice(
                      z_bit_diagonal_size
                    , x_bit_diagonal_size
                    , 0
                    , z_bit_diagonal_size
                  )
                , postConstraintForRegion(
                      space
                    , Z_matrix.slice(
                          x_bit_diagonal_size
                        , number_of_qubits
                        , 0
                        , x_bit_diagonal_size
                      )
                    , postConstraintForRegion(
                          space
                        , X_matrix.slice(
                              x_bit_diagonal_size
                            , number_of_qubits
                            , 0
                            , x_bit_diagonal_size
                          )
                        , initial_ties.first
                      )
                  )
              )
          )
        , postConstraintForRegion(
              space
            , Z_matrix.slice(
                  x_bit_diagonal_size
                , number_of_qubits
                , z_bit_diagonal_size
                , x_bit_diagonal_size
              )
            , initial_ties.second
          )
    );
}
//@+node:gcross.20110112003748.1554: *3* postWeightRowOrderingConstraintOnRegion
BoolVarArgs postWeightRowOrderingConstraintOnRegion(
      OperatorSpace& space
    , BoolMatrix region
    , BoolVarArgs initial_ties
) {
    IntVarArgs weights(space,region.height(),0,region.width());
    BOOST_FOREACH(const unsigned int i, irange(0u,(unsigned int)region.height())) {
        linear(space,region.row(i),IRT_EQ,weights[i]);
    }
    return postOrderingConstraint(
         space
        ,IntMatrix(
             weights
            ,weights.size()
            ,1
         )
        ,initial_ties
    );
}
//@-others

}
//@-leo
