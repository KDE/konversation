%define	desktop_vendor	rockers

Name: konversation
Summary: A user friendly IRC Client for KDE3.x
Version: 0.10
Release: 0
License: GPL
Group: Applications/Internet
Source: http://konversation.sourceforge.net/downloads/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
Url: http://konversation.sourceforge.net/
Requires: libpng, kdebase >= 3.0
BuildRequires: libpng-devel, kdelibs-devel, arts-devel, libjpeg-devel
BuildRequires:  XFree86-devel, zlib-devel, qt-devel >= 3.0.2

%description
A simple and easy to use IRC client for KDE with support for strikeout; 
multi-channel joins; away / unaway messages; ignore list functionality; (experimental) 
support for foreign language characters; auto-connect to server; optional timestamps 
to chat windows; configurable background colors and much more.

%prep
rm -rf %{buildroot}

%setup -q -n %{name}-%{version}
test -f Makefile.dist && make -f Makefile.dist

%build
%configure --with-xinerama
make %{_smp_mflags}

%install
%makeinstall

mkdir -p %{buildroot}%{_datadir}/applications
desktop-file-install --vendor %{desktop_vendor} --delete-original \
  --dir %{buildroot}%{_datadir}/applications                      \
  --add-category X-Red-Hat-Extra                                  \
  --add-category Application                                      \
  --add-category Network                                          \
  %{buildroot}%{_datadir}/applnk/Internet/%{name}.desktop
  
rm -rf %{buildroot}%{_datadir}/applnk

%clean
rm -rf %{buildroot}

%post

%postun

%files
%defattr(-,root,root,-)
%doc AUTHORS COPYING README INSTALL
%{_bindir}/%{name}*
%{_datadir}/apps/%{name}/*
%{_datadir}/applications/%{desktop_vendor}-%{name}.desktop
%{_datadir}/locale/*/LC_MESSAGES/%{name}*
%{_datadir}/icons/locolor/*/*/%{name}*

%changelog
* Fri Feb 7 2003 Robert Rockers <brockers at dps.state.ok.us> 0.9
- Initial RedHat RPM release.


