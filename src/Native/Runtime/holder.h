//
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//

// -----------------------------------------------------------------------------------------------------------
// Cut down versions of the Holder and Wrapper template classes used in the CLR. If this coding pattern is
// also common in the Redhawk code then it might be worth investigating pulling the whole holder.h header file
// over (a quick look indicates it might not drag in too many extra dependencies).
//

// -----------------------------------------------------------------------------------------------------------
// This version of holder does not have a default constructor.

template <typename TYPE, void (*ACQUIRE_FUNC)(TYPE), void (*RELEASE_FUNC)(TYPE)>
class HolderNoDefaultValue
{
public:
    HolderNoDefaultValue(TYPE value, bool fTake = true) : m_value(value), m_held(false) 
        { if (fTake) { ACQUIRE_FUNC(value); m_held = true; } }

    ~HolderNoDefaultValue() { if (m_held) RELEASE_FUNC(m_value); }

    TYPE GetValue() { return m_value; }

    void Acquire() { ACQUIRE_FUNC(m_value); m_held = true; }
    void Release() { if (m_held) { RELEASE_FUNC(m_value); m_held = false; } }
    void SuppressRelease() { m_held = false; }
    TYPE Extract() { m_held = false; return GetValue(); }

protected:
    TYPE    m_value;
    bool    m_held;

private:
    // No one should be copying around holder types.
    HolderNoDefaultValue & operator=(const HolderNoDefaultValue & other);
    HolderNoDefaultValue(const HolderNoDefaultValue & other);
};

// -----------------------------------------------------------------------------------------------------------
template <typename TYPE, void (*ACQUIRE_FUNC)(TYPE), void (*RELEASE_FUNC)(TYPE), UIntNative DEFAULTVALUE = 0>
class Holder : public HolderNoDefaultValue<TYPE, ACQUIRE_FUNC, RELEASE_FUNC>
{
    typedef HolderNoDefaultValue<TYPE, ACQUIRE_FUNC, RELEASE_FUNC> MY_PARENT;
public:
    Holder() : MY_PARENT(DEFAULTVALUE, false) {}
    Holder(TYPE value, bool fTake = true) : MY_PARENT(value, fTake) {}

private:
    // No one should be copying around holder types.
    Holder & operator=(const Holder & other);
    Holder(const Holder & other);
};

// -----------------------------------------------------------------------------------------------------------
template <typename TYPE, void (*ACQUIRE_FUNC)(TYPE), void (*RELEASE_FUNC)(TYPE), UIntNative DEFAULTVALUE = 0>
class Wrapper : public Holder<TYPE, ACQUIRE_FUNC, RELEASE_FUNC, DEFAULTVALUE>
{
    typedef Holder<TYPE, ACQUIRE_FUNC, RELEASE_FUNC, DEFAULTVALUE> MY_PARENT;

public:
    Wrapper() : MY_PARENT() {}
    Wrapper(TYPE value, bool fTake = true) : MY_PARENT(value, fTake) {}

    FORCEINLINE TYPE& operator=(TYPE const & value)
    {
        Release();
        m_value = value;
        Acquire();
        return m_value;
    }

    FORCEINLINE const TYPE &operator->() { return m_value; }
    FORCEINLINE const TYPE &operator*() { return m_value; }
    FORCEINLINE operator TYPE() { return m_value; }

private:
    // No one should be copying around wrapper types.
    Wrapper & operator=(const Wrapper & other);
    Wrapper(const Wrapper & other);
};

// -----------------------------------------------------------------------------------------------------------
template <typename TYPE>
FORCEINLINE void DoNothing(TYPE value)
{
}

// -----------------------------------------------------------------------------------------------------------
template <typename TYPE> 
FORCEINLINE void Delete(TYPE *value)
{
    delete value;
}

// -----------------------------------------------------------------------------------------------------------
template <typename TYPE,
          typename PTR_TYPE = TYPE *,
          void (*ACQUIRE_FUNC)(PTR_TYPE) = DoNothing<typename PTR_TYPE>,
          void (*RELEASE_FUNC)(PTR_TYPE) = Delete<TYPE>,
          PTR_TYPE NULL_VAL = 0,
          typename BASE = Wrapper<PTR_TYPE, ACQUIRE_FUNC, RELEASE_FUNC, NULL_VAL> >
class NewHolder : public BASE
{
public:
    NewHolder(PTR_TYPE p = NULL_VAL) : BASE(p)
        { }

    PTR_TYPE& operator=(PTR_TYPE p)
        { return BASE::operator=(p); }

    bool IsNull()
        { return BASE::GetValue() == NULL_VAL; }
};

//-----------------------------------------------------------------------------
// NewArrayHolder : New []'ed pointer holder
//  {
//      NewArrayHolder<Foo> foo = new Foo [30];
//  } // delete [] foo on out of scope
//-----------------------------------------------------------------------------

template <typename TYPE> 
FORCEINLINE void DeleteArray(TYPE *value)
{
    delete [] value;
    value = NULL;
}

template <typename TYPE,
          typename PTR_TYPE = TYPE *,
          void (*ACQUIRE_FUNC)(PTR_TYPE) = DoNothing<typename PTR_TYPE>,
          void (*RELEASE_FUNC)(PTR_TYPE) = DeleteArray<TYPE>,
          PTR_TYPE NULL_VAL = 0,
          typename BASE = Wrapper<PTR_TYPE, ACQUIRE_FUNC, RELEASE_FUNC, NULL_VAL> >
class NewArrayHolder : public BASE
{
public:
    NewArrayHolder(PTR_TYPE p = NULL_VAL) : BASE(p)
        { }

    PTR_TYPE& operator=(PTR_TYPE p)
        { return BASE::operator=(p); }

    bool IsNull()
        { return BASE::GetValue() == NULL_VAL; }
};

// -----------------------------------------------------------------------------------------------------------
template<typename TYPE>
FORCEINLINE void Destroy(TYPE * value)
{
    value->Destroy();
}

// -----------------------------------------------------------------------------------------------------------
template <typename TYPE,
          typename PTR_TYPE = TYPE *,
          void (*ACQUIRE_FUNC)(PTR_TYPE) = DoNothing<PTR_TYPE>,
          void (*RELEASE_FUNC)(PTR_TYPE) = Destroy<TYPE>,
          PTR_TYPE NULL_VAL = 0,
          typename BASE = Wrapper<PTR_TYPE, ACQUIRE_FUNC, RELEASE_FUNC, NULL_VAL> >
class CreateHolder : public BASE
{
public:
    CreateHolder(PTR_TYPE p = NULL_VAL) : BASE(p)
        { }

    PTR_TYPE& operator=(PTR_TYPE p)
        { return BASE::operator=(p); }
};

