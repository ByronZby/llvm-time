#ifndef GRAPHUTILS_H_INCLUDED_
#define GRAPHUTILS_H_INCLUDED_

#include <type_traits>
#include <utility>

/*
template <typename PtrType, typename BaseIterator>
class PointerCastIterator : public BaseIterator {
public:
    using value_type = PtrType;
    using pointer    = value_type *;
    using reference  = value_type &;

    constexpr PointerCastIterator(const BaseIterator & BI)
        : BaseIterator(BI) { }

    constexpr PointerCastIterator(BaseIterator && BI)
        : BaseIterator(std::move(BI)) { }

    value_type operator*() const {
        return static_cast<value_type>(BaseIterator::operator*());
    }
};
*/

template <typename FunType, FunType F, typename BaseIterator>
class CastIterator : public BaseIterator {
public:
    using value_type =
        typename std::result_of<FunType&(typename BaseIterator::value_type)>
                    ::type;
    using pointer    = value_type *;
    using reference  = value_type &;

    constexpr CastIterator(const BaseIterator & BI)
        : BaseIterator(BI) { }

    constexpr CastIterator(BaseIterator && BI)
        : BaseIterator(std::move(BI)) { }

    value_type operator*() const {
        return F(BaseIterator::operator*());
    }
};

template <typename PtrType>
constexpr PtrType castFromVoid(void * P) {
    return static_cast<PtrType>(P);
}

template <typename PtrType, typename BaseIterator>
class PointerCastIterator : public CastIterator<decltype(castFromVoid<PtrType>), castFromVoid<PtrType>, BaseIterator> {
    using BaseT = CastIterator<decltype(castFromVoid<PtrType>), castFromVoid<PtrType>, BaseIterator>;

public:
    constexpr PointerCastIterator(const BaseIterator & BI)
        : BaseT(BI) { }

    constexpr PointerCastIterator(BaseIterator && BI)
        : BaseT(BI) { }
};



#endif
