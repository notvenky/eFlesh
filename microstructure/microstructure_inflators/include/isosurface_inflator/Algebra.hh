// From Misha Kazhdan (Volumetric Basis Reduction for Global Seamless Parametrization project)
// but tweaked slightly.
// Note, these only work well with "Element" classes with value semantics (as
// opposed to reference semantics).
#ifndef ALGEBRA_INCLUDED
#define ALGEBRA_INCLUDED

// For this to work, Element must implement:
// void Element::Add(const Element& e)
// void Element::Scale(Real s)

// JP: pass-by-values used below are intentional--they can in some cases avoid
// the copy construction that is always required by accepting a const ref
// argument.
template<class Real, class Element>
class VectorSpace {
public:
    // Negation
    friend Element operator-(Element e) { e.Scale(-1); return e; }

    // Compound assignment operations
    friend Element& operator+=(Element &e1, const Element &e2) {               e1.Add(e2); return e1; }
    friend Element& operator-=(Element &e1,       Element  e2) { e2.Scale(-1); e1.Add(e2); return e1; }
    friend Element& operator*=(Element & e, const Real     &s) {  e.Scale(s);              return  e; }
    friend Element& operator/=(Element & e, const Real     &s) {  e.Scale(1.0 / s);        return  e; }

    // Binary operations
    friend Element operator+(const Element &e1, Element e2) {               e2.Add(e1); return e2; }
    friend Element operator-(const Element &e1, Element e2) { e2.Scale(-1); e2.Add(e1); return e2; }

    // Scaling operations
    friend Element operator*(const Real &s, Element e) { e.Scale(s);       return e; }
    friend Element operator*(Element e, const Real &s) { e.Scale(s);       return e; }
    friend Element operator/(Element e, const Real &s) { e.Scale(1.0 / s); return e; }
};

template<class Real, class Element>
class InnerProductSpace : public VectorSpace<Real, Element> {
public:
    // For this to work, Element must implement:
    // Real Element::InnerProduct   (const Element& e) const

    static Real SquareNorm     (const Element& e)                     { return e.InnerProduct(e);   }
    static Real Dot            (const Element& e1, const Element& e2) { return e1.InnerProduct(e2); }
    static Real SquareDistance (const Element& e1, const Element& e2) { return SquareNorm(e1-e2);   }
};

template<class Element>
class Group {
public:
    // For this to work, Element must implement:
    // void Element::SetIdentity    (void);
    // void Element::Multiply       (const Element& e)
    // void Element::Invert         (void)

    static Element Identity(void) {
        Element out;
        out.SetIdentity();
        return out;
    }

    Element inverse(void) const {
        Element out=*(const Element*)this;
        out.Invert();
        return out;
    }

    friend Element  operator*(const Element& e1, const Element& e2) {
        Element out=e1;
        out.Multiply(e2);
        return out;
    }

    friend Element operator/(const Element& e1, const Element& e2) {
        Element out=e1;
        Element inv=e2.Invert();
        out.Multiply(inv);
        return out;
    }

    friend Element& operator*=(Element& e1, const Element& e2) {
        e1.Multiply(e2);
        return e1;
    }

    friend Element& operator/=(Element& e1, const Element& e2) {
        Element inv=e2;
        inv.Invert();
        e1.Multiply(inv);
        return e1;
    }
};

template<class Real, class Element>
class Algebra : public VectorSpace<Real, Element> {
public:
    virtual void SetIdentity    (void)              = 0;
    virtual void Multiply       (const Element& e)  = 0;

    static Element Identity(void) {
        Element out;
        out.SetIdentity();
        return out;
    }

    friend Element  operator*(const Element& e1, const Element& e2) {
        Element out=e1;
        out.Multiply(e2);
        return out;
    }

    friend Element& operator*=(Element& e1, const Element& e2) {
        e1.Multiply(e2);
        return e1;
    }
};

template< class Element >
class Field {
public:
    // For this to work, need to define:
    // void Element::SetAdditiveIdentity        (void);
    // void Element::SetMultiplicativeIdentity  (void);
    // void Element::Add                        (const Element& e);
    // void Element::Multiply                   (const Element& e);
    // void Element::Negate                     (void);
    // void Element::Invert                     (void);

    static Element AdditiveIdentity(void) {
        Element out;
        out.SetAdditiveIdentity();
        return out;
    }

    static Element MultiplicativeIdentity(void) {
        Element out;
        out.SetMultiplicativeIdentity();
        return out;
    }
    Element additiveInverse(void) const {
        Element out=*(const Element*)this;
        out.Negate();
        return out;
    }

    Element multiplicativeInverse(void) const {
        Element out=*(const Element*)this;
        out.Invert();
        return out;
    }

    friend Element operator+(const Element& e1, const Element& e2) {
        Element out=e1;
        out.Add(e2);
        return out;
    }

    friend Element operator*(const Element& e1, const Element& e2) {
        Element out=e1;
        out.Multiply(e2);
        return out;
    }

    friend Element operator-(const Element& e1, const Element& e2) {
        Element out=e1;
        Element inv=e2.Negate();
        out.Add(inv);
        return out;
    }

    friend Element operator/(const Element& e1, const Element& e2) {
        Element out=e1;
        Element inv=e2.Invert();
        out.Multiply(inv);
        return out;
    }

    friend Element& operator+=(Element& e1, const Element& e2) {
        e1.Add(e2);
        return e1;
    }

    friend Element& operator*=(Element& e1, const Element& e2) {
        e1.Multiply(e2);
        return e1;
    }

    friend Element& operator-=(Element& e1, const Element& e2) {
        Element inv=e2;
        inv.Negate();
        e1.Add(inv);
        return e1;
    }

    friend Element& operator/=(Element& e1, const Element& e2) {
        Element inv=e2;
        inv.Invert();
        e1.Multiply(inv);
        return e1;
    }
};

#endif // ALGEBRA_INCLUDED
