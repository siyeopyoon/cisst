/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-    */
/* ex: set filetype=cpp softtabstop=4 shiftwidth=4 tabstop=4 cindent expandtab: */

/*
  $Id: $

  Author(s):  Daniel Li, Anton Deguet, Mitch Williams
  Created on: 2009-05-20

  (C) Copyright 2009 Johns Hopkins University (JHU), All Rights
  Reserved.

--- begin cisst license - do not edit ---

This software is provided "as is" under an open source license, with
no warranty.  The complete license can be found in license.txt and
http://www.cisst.org/cisst/license.txt.

--- end cisst license ---

*/

/*****************************************************************************
 PLACEHOLDER STRINGS TO LOOK FOR:

   TODO
*****************************************************************************/

#define CISST_EXPORT
#define CISST_DEPRECATED

%include "std_streambuf.i"
%include "std_iostream.i"

%rename(__str__) ToString;


%init %{
        import_array()   /* Initial function for NumPy */
%}


/******************************************************************************
  TYPEMAPS (in, out) FOR vctDynamicVector
******************************************************************************/


%typemap(in) vctDynamicVector
{
    /*****************************************************************************
    *   %typemap(in) vctDynamicVector
    *   Passing a vctDynamicVector by copy
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this type map.
    *****************************************************************************/

    /*****************************************************************************
     CHECK IF THE PYTHON OBJECT (NAMED `$input') THAT WAS PASSED TO THIS TYPE MAP
     IS A PYARRAY, IS OF THE CORRECT DTYPE, AND IS ONE-DIMENSIONAL
    *****************************************************************************/

    if (!(   vctThrowUnlessIsPyArray($input)
          && vctThrowUnlessIsSameTypeArray<$1_ltype::value_type>($input)
          && vctThrowUnlessDimension1($input)
          && vctThrowUnlessCorrectVectorSize($input, $1))
        ) {
          return NULL;
    }

    /*****************************************************************************
     COPY THE DATA OF THE PYARRAY (NAMED `$input') TO THE vctDynamicVector
    *****************************************************************************/

    // Create a temporary vctDynamicVectorRef container
    const npy_intp size = PyArray_DIM($input, 0);
    const npy_intp stride = PyArray_STRIDE($input, 0) / sizeof($1_ltype::value_type);
    const $1_ltype::pointer data = reinterpret_cast<$1_ltype::pointer>(PyArray_DATA($input));

    const vctDynamicVectorRef<$1_ltype::value_type> tempContainer(size, data, stride);

    // Copy the data from the temporary container to the vctDynamicVector
    $1.ForceAssign(tempContainer);
}


%typemap(out) vctDynamicVector
{
    /*****************************************************************************
    *   %typemap(out) vctDynamicVector
    *   Returning a vctDynamicVector
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this type map.
    *****************************************************************************/

    /*****************************************************************************
     CREATE A NEW PYARRAY OBJECT
    *****************************************************************************/

    //Create a new PyArray and set its size
    npy_intp* sizes = PyDimMem_NEW(1);
    sizes[0] = $1.size();
    int type = vctPythonType<$1_ltype::value_type>();
    $result = PyArray_SimpleNew(1, sizes, type);

    /*****************************************************************************
     COPY THE DATA FROM THE vctDynamicConstVectorRef TO THE PYARRAY
    *****************************************************************************/

    // Create a temporary vctDynamicVectorRef container
    const npy_intp size = $1.size();
    const npy_intp stride = 1;
    const $1_ltype::pointer data = reinterpret_cast<$1_ltype::pointer>(PyArray_DATA($result));

    vctDynamicVectorRef<$1_ltype::value_type> tempContainer(size, data, stride);

    // Copy the data from the vctDynamicConstVectorRef to the temporary container
    tempContainer.Assign($1);
}


/******************************************************************************
  TYPEMAPS (in, argout, out) FOR vctDynamicVector &
******************************************************************************/

%typemap(in) vctDynamicVector &
{
    /*****************************************************************************
    *   %typemap(in) vctDynamicVector &
    *   Passing a vctDynamicVector by reference
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this typemap.
    *****************************************************************************/

    /*****************************************************************************
     CHECK IF THE PYTHON OBJECT (NAMED `$input') THAT WAS PASSED TO THIS TYPE MAP
     IS A PYARRAY, IS OF THE CORRECT DTYPE, IS ONE-DIMENSIONAL, AND IS WRITABLE
    *****************************************************************************/

    if (!(   vctThrowUnlessIsPyArray($input)
          && vctThrowUnlessIsSameTypeArray<$*1_ltype::value_type>($input)
          && vctThrowUnlessDimension1($input)
          && vctThrowUnlessCorrectVectorSize($input, *($1))
          && vctThrowUnlessIsWritable($input)
          && vctThrowUnlessOwnsData($input, *($1))
          && vctThrowUnlessNotReferenced($input, *($1)))
        ) {
          return NULL;
    }

    /*****************************************************************************
     COPY THE DATA OF THE PYARRAY (NAMED `$input') TO THE vctDynamicVector
    *****************************************************************************/

    // Create a temporary vctDynamicVectorRef container
    const npy_intp size = PyArray_DIM($input, 0);
    const npy_intp stride = PyArray_STRIDE($input, 0) / sizeof($*1_ltype::value_type);
    const $*1_ltype::pointer data = reinterpret_cast<$*1_ltype::pointer>(PyArray_DATA($input));

    const vctDynamicVectorRef<$*1_ltype::value_type> tempContainer(size, data, stride);

    // Create the vctDynamicVector
    $1 = new $*1_ltype(tempContainer);
}


%typemap(argout) vctDynamicVector &
{
    /*****************************************************************************
    *   %typemap(argout) vctDynamicVector &
    *   Passing a vctDynamicVector by reference
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this type map.
    *****************************************************************************/

    /*************************************************************************
     CHECK IF THE CONTAINER HAS BEEN RESIZED
    *************************************************************************/

    const $*1_ltype::size_type input_size = PyArray_DIM($input, 0);
    const $*1_ltype::size_type output_size = $1->size();

    if (input_size != output_size) {
        // Resize the PyArray by:
        //  1)  Creating an array containing the new size
        //  2)  Pass that array to the resizing function given by NumPy API
        npy_intp *sizes = PyDimMem_NEW(1);              // create an array of sizes; dimension 1
        sizes[0] = output_size;                         // set the size
        PyArray_Dims dims;                              // create a PyArray_Dims object to hand to PyArray_Resize
        dims.ptr = sizes;
        dims.len = 1;
        PyArray_Resize((PyArrayObject *) $input, &dims, 0, NPY_CORDER);
    }

    /*************************************************************************
     COPY THE DATA TO THE PYARRAY
    *************************************************************************/

    // Create a temporary vctDynamicVectorRef container
    const npy_intp size = PyArray_DIM($input, 0);
    const npy_intp stride = PyArray_STRIDE($input, 0) / sizeof($*1_ltype::value_type);
    const $*1_ltype::pointer data = reinterpret_cast<$*1_ltype::pointer>(PyArray_DATA($input));

    vctDynamicVectorRef<$*1_ltype::value_type> tempContainer(size, data, stride);

    // Copy the data from the temporary container to the vctDynamicVector
    tempContainer.Assign(*($1));

    /*************************************************************************
     CLEAN UP
    *************************************************************************/

    // Don't forget to free the memory we allocated in the `in' typemap
    delete $1;
}


%typemap(out) vctDynamicVector &
{
    /* Return vector by reference
       Using: %typemap(out) vctDynamicVector &
     */

    //Create new size array and set size
    npy_intp* sizeOfReturnedVector = PyDimMem_NEW(1);
    sizeOfReturnedVector[0] = $1->size();

    //create a new PyArray from the reference returned by the C function
    int type = vctPythonType<$*1_ltype::value_type>();
    $result = PyArray_SimpleNewFromData(1, sizeOfReturnedVector, type,  $1->Pointer() );
}


/******************************************************************************
  TYPEMAPS (in, argout, out) FOR const vctDynamicVector &
******************************************************************************/


%typemap(in) const vctDynamicVector &
{
    /*****************************************************************************
    *   %typemap(in) const vctDynamicVector &
    *   Passing a vctDynamicVector by const &
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this type map.
    *****************************************************************************/

    /*****************************************************************************
     CHECK IF THE PYTHON OBJECT (NAMED `$input') THAT WAS PASSED TO THIS TYPE MAP
     IS A PYARRAY, IS OF THE CORRECT DTYPE, AND IS ONE-DIMENSIONAL
    *****************************************************************************/

    if (!(   vctThrowUnlessIsPyArray($input)
          && vctThrowUnlessIsSameTypeArray<$*1_ltype::value_type>($input)
          && vctThrowUnlessDimension1($input)
          && vctThrowUnlessCorrectVectorSize($input, *($1)))
        ) {
          return NULL;
    }

    /*****************************************************************************
     COPY THE DATA OF THE PYARRAY (NAMED `$input') TO THE vctDynamicVector
    *****************************************************************************/

    // Create a temporary vctDynamicVectorRef container
    const npy_intp size = PyArray_DIM($input, 0);
    const npy_intp stride = PyArray_STRIDE($input, 0) / sizeof($*1_ltype::value_type);
    const $*1_ltype::pointer data = reinterpret_cast<$*1_ltype::pointer>(PyArray_DATA($input));

    const vctDynamicVectorRef<$*1_ltype::value_type> tempContainer(size, data, stride);

    // Create the vctDynamicVector
    $1 = new $*1_ltype(tempContainer);
}


%typemap(argout) const vctDynamicVector &
{
    /**************************************************************************
    *   %typemap(argout) const vctDynamicVector &
    *   Passing a vctDynamicVector by const reference
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this type map.
    **************************************************************************/

    /**************************************************************************
     CLEAN UP
    **************************************************************************/

    // Don't forget to free the memory we allocated in the `in' typemap
    delete $1;
}


%typemap(out) const vctDynamicVector &
{
    /* Return vector by const reference
       Using: %typemap(out) const vctDynamicVector &
     */

    /* To imitate const functionality, set the writable flag to false */

    //Create new size array and set size
    npy_intp* sizes = PyDimMem_NEW(1);
    sizes[0] = $1->size();

    // Look at the NumPy C API to see how these lines work: http://projects.scipy.org/numpy/wiki/NumPyCAPI
    int type = vctPythonType<$*1_ltype::value_type>();
    PyArray_Descr *descr = PyArray_DescrFromType(type);
    $result = PyArray_NewFromDescr(&PyArray_Type, descr, 1, sizes, NULL, $1->Pointer(), NPY_CARRAY_RO, NULL);
}


/******************************************************************************
  TYPEMAPS (in, out) FOR vctDynamicVectorRef
******************************************************************************/


%typemap(in) vctDynamicVectorRef
{
    /*************************************************************************
    *   %typemap(in) vctDynamicVectorRef
    *   Passing a vctDynamicVectorRef by copy
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic
    *   behind this typemap.
    *************************************************************************/

    /*************************************************************************
     CHECK IF THE PYTHON OBJECT (NAMED `$input') THAT WAS PASSED TO THIS
     TYPEMAP IS A PYARRAY, IS OF THE CORRECT DTYPE, IS ONE-DIMENSIONAL, AND
     IS WRITABLE
    *************************************************************************/

    if (!(   vctThrowUnlessIsPyArray($input)
          && vctThrowUnlessIsSameTypeArray<$1_ltype::value_type>($input)
          && vctThrowUnlessDimension1($input)
          && vctThrowUnlessCorrectVectorSize($input, $1)
          && vctThrowUnlessIsWritable($input))
        ) {
          return NULL;
    }

    /*************************************************************************
     SET THE SIZE, STRIDE AND DATA POINTER OF THE vctDynamicVectorRef
     OBJECT (NAMED `$1') TO MATCH THAT OF THE PYARRAY (NAMED `$input')
    *************************************************************************/

    const npy_intp size = PyArray_DIM($input, 0);
    const npy_intp stride = PyArray_STRIDE($input, 0) / sizeof($1_ltype::value_type);
    const $1_ltype::pointer data = reinterpret_cast<$1_ltype::pointer>(PyArray_DATA($input));

    $1.SetRef(size, data, stride);
}



%typemap(out, optimal="1") vctDynamicVectorRef
{
    /*****************************************************************************
    *   %typemap(out, optimal="1") vctDynamicVectorRef
    *   Returning a vctDynamicVectorRef
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this type map.
    *****************************************************************************/

    /*****************************************************************************
     CREATE A NEW PYARRAY OBJECT
    *****************************************************************************/

    npy_intp *sizes = PyDimMem_NEW(1);
    sizes[0] = $1.size();
    int type = vctPythonType<$1_ltype::value_type>();
    $result = PyArray_SimpleNew(1, sizes, type);

    /*****************************************************************************
     COPY THE DATA FROM THE vctDynamicVectorRef TO THE PYARRAY
    *****************************************************************************/

    // Create a temporary vctDynamicVectorRef container
    const npy_intp size = $1.size();
    const npy_intp stride = 1;
    const $1_ltype::pointer data = reinterpret_cast<$1_ltype::pointer>(PyArray_DATA($result));

    vctDynamicVectorRef<$1_ltype::value_type> tempContainer(size, data, stride);

    // Copy the data from the vctDynamicVectorRef to the temporary container
    tempContainer.Assign($1);
}


/******************************************************************************
  TYPEMAPS (in, argout) FOR const vctDynamicVectorRef &
******************************************************************************/


%typemap(in) const vctDynamicVectorRef &
{
    /**************************************************************************
    *   %typemap(in) const vctDynamicVectorRef &
    *   Passing a vctDynamicVectorRef by const reference
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this type map.
    **************************************************************************/

    /*****************************************************************************
     CHECK IF THE PYTHON OBJECT (NAMED `$input') THAT WAS PASSED TO THIS TYPE MAP
     IS A PYARRAY, IS OF THE CORRECT DTYPE, AND IS ONE-DIMENSIONAL
    *****************************************************************************/

    if (!(   vctThrowUnlessIsPyArray($input)
          && vctThrowUnlessIsSameTypeArray<$*1_ltype::value_type>($input)
          && vctThrowUnlessDimension1($input)
          && vctThrowUnlessCorrectVectorSize($input, *($1)))
        ) {
          return NULL;
    }

    /*****************************************************************************
     CREATE A vctDynamicVectorRef TO POINT TO THE DATA OF THE PYARRAY
    *****************************************************************************/

    // Create the vctDynamicVectorRef
    const npy_intp size = PyArray_DIM($input, 0);
    const npy_intp stride = PyArray_STRIDE($input, 0) / sizeof($*1_ltype::value_type);
    const $*1_ltype::pointer data = reinterpret_cast<$*1_ltype::pointer>(PyArray_DATA($input));

    $1 = new $*1_ltype(size, data, stride);
}


%typemap(argout) const vctDynamicVectorRef &
{
    /**************************************************************************
    *   %typemap(argout) const vctDynamicVectorRef &
    *   Passing a vctDynamicVectorRef by const reference
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this type map.
    **************************************************************************/

    /**************************************************************************
     CLEAN UP
    **************************************************************************/

    // Don't forget to free the memory we allocated in the `in' typemap
    delete $1;
}


/******************************************************************************
  TYPEMAPS (in, out) FOR vctDynamicConstVectorRef
******************************************************************************/


%typemap(in) vctDynamicConstVectorRef
{
    /*****************************************************************************
    *   %typemap(in) vctDynamicConstVectorRef
    *   Passing a vctDynamicConstVectorRef by copy
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this type map.
    *****************************************************************************/

    /*****************************************************************************
     CHECK IF THE PYTHON OBJECT (NAMED `$input') THAT WAS PASSED TO THIS TYPE MAP
     IS A PYARRAY, IS OF THE CORRECT DTYPE, AND IS ONE-DIMENSIONAL
    *****************************************************************************/

    if (!(   vctThrowUnlessIsPyArray($input)
          && vctThrowUnlessIsSameTypeArray<$1_ltype::value_type>($input)
          && vctThrowUnlessDimension1($input)
          && vctThrowUnlessCorrectVectorSize($input, $1))
        ) {
          return NULL;
    }

    /*****************************************************************************
     SET THE SIZE, STRIDE, AND DATA POINTER OF THE
     vctDynamicConstVectorRef OBJECT (NAMED `$1') TO MATCH THAT OF THE
     PYARRAY (NAMED `$input')
    *****************************************************************************/

    const npy_intp size = PyArray_DIM($input, 0);
    const npy_intp stride = PyArray_STRIDE($input, 0) / sizeof($1_ltype::value_type);
    const $1_ltype::pointer data = reinterpret_cast<$1_ltype::pointer>(PyArray_DATA($input));

    $1.SetRef(size, data, stride);
}


// TODO: Why does this out typemap work without the ``optimal'' flag?
%typemap(out) vctDynamicConstVectorRef
{
    /*****************************************************************************
    *   %typemap(out) vctDynamicConstVectorRef
    *   Returning a vctDynamicConstVectorRef
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this type map.
    *****************************************************************************/

    /*****************************************************************************
     CREATE A NEW PYARRAY OBJECT
    *****************************************************************************/

    npy_intp *sizes = PyDimMem_NEW(1);
    sizes[0] = $1.size();

    // Look at the NumPy C API to see how these lines work: http://projects.scipy.org/numpy/wiki/NumPyCAPI
    int type = vctPythonType<$1_ltype::value_type>();
    PyArray_Descr *descr = PyArray_DescrFromType(type);
    $result = PyArray_NewFromDescr(&PyArray_Type, descr,  1, sizes, NULL, NULL, NPY_CARRAY, NULL);
    PyArray_FLAGS($result) &= ~NPY_WRITEABLE;

    /*****************************************************************************
     COPY THE DATA FROM THE vctDynamicConstVectorRef TO THE PYARRAY
    *****************************************************************************/

    // Create a temporary vctDynamicVectorRef container
    const npy_intp size = $1.size();
    const npy_intp stride = 1;
    const $1_ltype::pointer data = reinterpret_cast<$1_ltype::pointer>(PyArray_DATA($result));

    vctDynamicVectorRef<$1_ltype::value_type> tempContainer(size, data, stride);

    // Copy the data from the vctDynamicConstVectorRef to the temporary container
    tempContainer.Assign($1);
}


/******************************************************************************
  TYPEMAPS (in, argout) FOR const vctDynamicConstVectorRef &
******************************************************************************/


%typemap(in) const vctDynamicConstVectorRef &
{
    /**************************************************************************
    *   %typemap(in) const vctDynamicConstVectorRef &
    *   Passing a vctDynamicConstVectorRef by const reference
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this type map.
    **************************************************************************/

    /*****************************************************************************
     CHECK IF THE PYTHON OBJECT (NAMED `$input') THAT WAS PASSED TO THIS TYPE MAP
     IS A PYARRAY, IS OF THE CORRECT DTYPE, AND IS ONE-DIMENSIONAL
    *****************************************************************************/

    if (!(   vctThrowUnlessIsPyArray($input)
          && vctThrowUnlessIsSameTypeArray<$*1_ltype::value_type>($input)
          && vctThrowUnlessDimension1($input)
          && vctThrowUnlessCorrectVectorSize($input, *($1)))
        ) {
          return NULL;
    }

    /*****************************************************************************
     CREATE A vctDynamicConstVectorRef TO POINT TO THE DATA OF THE PYARRAY
    *****************************************************************************/

    // Create the vctDynamicConstVectorRef
    const npy_intp size = PyArray_DIM($input, 0);
    const npy_intp stride = PyArray_STRIDE($input, 0) / sizeof($*1_ltype::value_type);
    const $*1_ltype::pointer data = reinterpret_cast<$*1_ltype::pointer>(PyArray_DATA($input));

    $1 = new $*1_ltype(size, data, stride);
}


%typemap(argout) const vctDynamicConstVectorRef &
{
    /**************************************************************************
    *   %typemap(argout) const vctDynamicConstVectorRef &
    *   Passing a vctDynamicConstVectorRef by const reference
    *
    *   See the documentation ``Developer's Guide to Writing Typemaps'' for documentation on the logic behind
    *   this type map.
    **************************************************************************/

    /**************************************************************************
     CLEAN UP
    **************************************************************************/

    // Don't forget to free the memory we allocated in the `in' typemap
    delete $1;
}


/**************************************************************************
*                          Applying Typemaps
**************************************************************************/

%define VCT_TYPEMAPS_APPLY_DYNAMIC_VECTORS(elementType)
%apply vctDynamicVector         {vctDynamicVector<elementType>};
%apply vctDynamicVector &       {vctDynamicVector<elementType> &};
%apply const vctDynamicVector & {const vctDynamicVector<elementType> &};
%apply vctDynamicVectorRef         {vctDynamicVectorRef<elementType>};
%apply const vctDynamicVectorRef & {const vctDynamicVectorRef<elementType> &};
%apply vctDynamicConstVectorRef         {vctDynamicConstVectorRef<elementType>};
%apply const vctDynamicConstVectorRef & {const vctDynamicConstVectorRef<elementType> &};
%enddef

VCT_TYPEMAPS_APPLY_DYNAMIC_VECTORS(int);
VCT_TYPEMAPS_APPLY_DYNAMIC_VECTORS(double);
