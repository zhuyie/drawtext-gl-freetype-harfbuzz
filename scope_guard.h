#ifndef __SCOPE_GUARD_H__
#define __SCOPE_GUARD_H__

#include <algorithm>

// ScopeGuard for C++11, Andrei Alexandrescu
// https://skydrive.live.com/view.aspx?resid=F1B8FF18A2AEC5C5!1158&app=WordPdf&authkey=!APo6bfP5sJ8EmH4
template<class Fun>
class ScopeGuard {
    Fun f_;
    bool active_;
public:
    ScopeGuard(Fun f)
        : f_(std::move(f))
        , active_(true)
    {
    }
    ~ScopeGuard()
    {
        if (active_)
            f_();
    }
    void dismiss()
    {
        active_ = false;
    }
    ScopeGuard() = delete;
    ScopeGuard(const ScopeGuard &) = delete;
    ScopeGuard& operator=(const ScopeGuard &) = delete;
    ScopeGuard(ScopeGuard&&rhs)
            : f_(std::move(rhs.f_)),
              active_(rhs.active_)
    {
        rhs.dismiss();
    }
};

template<class Fun>
ScopeGuard<Fun> scopeGuard(Fun f)
{
    return ScopeGuard<Fun>(std::move(f));
}

#endif // !__SCOPE_GUARD_H__
