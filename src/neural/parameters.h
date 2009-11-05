/* parameters.h                                                    -*- C++ -*-
   Jeremy Barnes, 2 November 2009
   Copyright (c) 2009 Jeremy Barnes.  All rights reserved.

   Desciption of parameters.  Used to allow polymorphic updates of
   parameters.
*/

#ifndef __neural__parameters_h__
#define __neural__parameters_h__

#include <vector>
#include <boost/multi_array.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include "utils/hash_map.h"
#include "boosting/thread_context.h"
#include "db/persistent_fwd.h"
#include "stats/distribution.h"


namespace ML {

class Layer;

struct Parameter_Value : boost::noncopyable {
    Parameter_Value(const std::string & name)
        : name_(name)
    {
    }

    virtual ~Parameter_Value() {}

    virtual size_t num_parameters() const = 0;
    
    std::string name() const { return name_; }

protected:
    std::string name_;
};

template<typename Underlying>
struct Vector_Ref : public Parameter_Value {

    Vector_Ref(const std::string & name,
               const Underlying * array, size_t size)
        : Parameter_Value(name), array_(array), size_(size)
    {
    }

    virtual size_t num_parameters() const
    {
        return size_;
    }
    
protected:
    const Underlying * array_;
    size_t size_;
};

template<typename Underlying>
struct Matrix_Ref : public Parameter_Value {

    Matrix_Ref(const std::string & name,
               const Underlying * array, size_t size1, size_t size2)
        : Parameter_Value(name),
          array_(array), size1_(size1), size2_(size2)
    {
    }

    virtual size_t num_parameters() const
    {
        return size1_ * size2_;
    }
    
protected:
    const Underlying * array_;
    size_t size1_, size2_;
};


/*****************************************************************************/
/* PARAMETERS                                                                */
/*****************************************************************************/

struct Parameters {

    /** Add a vector of values to the parameters */

    Parameters & add(Parameter_Value * param);

    template<class Float>
    Parameters &
    add(const std::string & name,
        std::vector<Float> & values)
    {
        return add(new Vector_Ref<Float>(name, &values[0], values.size()));
    }
    
    /** Add a matrix of values to the parameters */
    template<class Float>
    Parameters &
    add(const std::string & name,
        boost::multi_array<Float, 2> & values)
    {
        return add(new Matrix_Ref<Float>(name, values.data(),
                                         values.shape()[0], values.shape()[1]));
    }

    void serialize(DB::Store_Writer & store) const;

    /** Reconstitutes the object, not the parameters.  To reconstitute the
        parameters, first reconstitute a new object and then assign the
        new version. */
    void reconstitute(DB::Store_Reader & store);

    void fill(float value);

    void random_fill(float limit, Thread_Context & context);
    
    void operator -= (const Parameters & other);

    void operator += (const Parameters & other);

    double two_norm() const;

    void operator *= (double value);

    virtual void update(const Parameters & other, double learning_rate);

protected:
    Parameters();
    Parameters(const Parameters & other);

    Parameters & operator = (const Parameters & other);

    void swap(Parameters & other) const;

    std::hash_map<std::string, int> by_name;
    boost::ptr_vector<Parameter_Value> params;
};


/*****************************************************************************/
/* PARAMETER_REF                                                             */
/*****************************************************************************/

/** Parameters that are stored somewhere else but referenced here. */

template<class Float>
struct Parameter_Ref : public Parameters {
};


/*****************************************************************************/
/* PARAMETER_COPY                                                            */
/*****************************************************************************/

/** Storage of a value for each parameter, in the given type. */

template<class Float>
struct Parameter_Copy : public Parameters {
    Parameter_Copy();

    Parameter_Copy(const Parameters & other);

    Parameter_Copy(const Layer & layer);

protected:
    // The actual values, stored contiguously for efficiency.
    distribution<Float> values;
};

} // namespace ML

#endif /* __neural__parameters_h__ */
