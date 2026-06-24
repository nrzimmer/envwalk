%global debug_package %{nil}

Name:           envwalk
Version:        0.3
Release:        3%{?dist}
Summary:        Per-directory .env loader for zsh and bash

License:        MIT
URL:            https://github.com/nrzimmer/envwalk
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  binutils
BuildRequires:  libasan
BuildRequires:  libubsan

%description
envwalk automatically loads and unloads .env files when entering or
leaving an allowed directory, for the zsh and bash shells.

%prep
%setup -q

%build
make release

%check
make test

%install
install -Dm755 envwalk %{buildroot}%{_bindir}/%{name}

%files
%{_bindir}/%{name}

%changelog
* Thu Apr 23 2026 Natanael Rodrigo Zimmer <nrzimmer@gmail.com> - 0.3-3
- fix stack trace
- run tests with debug and release configuration

* Thu Apr 23 2026 Natanael Rodrigo Zimmer <nrzimmer@gmail.com> - 0.3-2
- better function to get current path

* Fri Apr 17 2026 Natanael Rodrigo Zimmer <nrzimmer@gmail.com> - 0.3-1
- fix ubuntu packaging && GCC 13 support
- update readme

* Thu Apr 16 2026 Natanael Rodrigo Zimmer <nrzimmer@gmail.com> - 0.2-1
- Initial release.
