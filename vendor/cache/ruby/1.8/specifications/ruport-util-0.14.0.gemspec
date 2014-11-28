# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "ruport-util"
  s.version = "0.14.0"

  s.required_rubygems_version = nil if s.respond_to? :required_rubygems_version=
  s.authors = ["Gregory Brown"]
  s.cert_chain = nil
  s.date = "2008-04-02"
  s.description = "ruport-util provides a number of utilities and helper libs for Ruby Reports"
  s.email = "  gregory.t.brown@gmail.com"
  s.executables = ["rope", "csv2ods"]
  s.extra_rdoc_files = ["INSTALL"]
  s.files = ["bin/rope", "bin/csv2ods", "INSTALL"]
  s.homepage = "http://code.rubyreports.org"
  s.rdoc_options = ["--title", "ruport-util Documentation", "--main", "INSTALL", "-q"]
  s.require_paths = ["lib"]
  s.required_ruby_version = Gem::Requirement.new("> 0.0.0")
  s.rubyforge_project = "ruport"
  s.rubygems_version = "1.8.19"
  s.summary = "A set of tools and helper libs for Ruby Reports"

  if s.respond_to? :specification_version then
    s.specification_version = 1

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<ruport>, [">= 1.6.0"])
      s.add_runtime_dependency(%q<mailfactory>, [">= 1.2.3"])
      s.add_runtime_dependency(%q<rubyzip>, [">= 0.9.1"])
    else
      s.add_dependency(%q<ruport>, [">= 1.6.0"])
      s.add_dependency(%q<mailfactory>, [">= 1.2.3"])
      s.add_dependency(%q<rubyzip>, [">= 0.9.1"])
    end
  else
    s.add_dependency(%q<ruport>, [">= 1.6.0"])
    s.add_dependency(%q<mailfactory>, [">= 1.2.3"])
    s.add_dependency(%q<rubyzip>, [">= 0.9.1"])
  end
end
