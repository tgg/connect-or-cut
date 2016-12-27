%define sname connect-or-cut
%define libname lib%{sname}
%define libver 1
%define debug_package %{nil}
%ifarch i386
%define bits 32
%else
%define bits 64
%endif

Name:           %{libname}%{libver}
Version:        1.0.2
Release:        1%{?dist}
Summary:        Stateless LD_PRELOAD based poor man's firewall

Group:          System Environment/Libraries
License:        BSD
URL:            https://github.com/tgg/%{sname}
Source0:        https://github.com/tgg/archive/%{sname}/%{sname}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{sname}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
connect-or-cut is a small library to interpose with LD_PRELOAD to a program
to prevent it from connecting where it should not.

This is similar to a firewall, except that:
 - you do not need to be root to use it
 - only processes launched after LD_PRELOAD is set are affected, not the full
   system

%ifarch noarch
%package -n %{sname}
Summary:        Stateless LD_PRELOAD based poor man's firewall
BuildArch:	noarch
Group:          System Environment/Libraries
Requires:	%{libname}%{libver} = %{version}-%{release}

%description -n connect-or-cut
This package contains the 'coc' helper script to use the library.
%endif

%prep
%setup -q -n %{sname}-%{version}


%build
%ifnarch noarch
CFLAGS="-O2 -g -Wall" make %{?_smp_mflags} os=$(uname -s) bits=%{bits}
%endif


%install
rm -rf %{buildroot}
%ifarch noarch
mkdir -p %{buildroot}%{_bindir}
install -m755 coc %{buildroot}%{_bindir}
%else
make install DESTBIN=%{buildroot}%{_bindir} DESTLIB=%{buildroot}%{_libdir}
rm %{buildroot}%{_bindir}/coc
%endif


%clean
rm -rf %{buildroot}


%ifarch noarch
%files -n %{sname}
%defattr(-,root,root,-)
%doc README.md LICENSE
%attr(755,-,-) %{_bindir}/coc
%else
%files
%defattr(-,root,root,-)
%{_libdir}/*
%endif


%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig


%changelog
* Mon Nov 01 2016 Thomas Girard <thomas.g.girard@free.fr> - 1.0.1-1
- Initial version of the package
