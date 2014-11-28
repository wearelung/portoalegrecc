# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "pdf-writer"
  s.version = "1.1.8"

  s.required_rubygems_version = nil if s.respond_to? :required_rubygems_version=
  s.authors = ["Austin Ziegler"]
  s.autorequire = "pdf/writer"
  s.cert_chain = nil
  s.date = "2008-03-16"
  s.description = "This library provides the ability to create PDF documents using only native Ruby libraries. There are several demo programs available in the demo/ directory. The canonical documentation for PDF::Writer is \"manual.pdf\", which can be generated using bin/techbook (just \"techbook\" for RubyGem users) and the manual file \"manual.pwd\"."
  s.email = "austin@rubyforge.org"
  s.executables = ["techbook"]
  s.extra_rdoc_files = ["README", "ChangeLog", "LICENCE"]
  s.files = ["bin/techbook", "README", "ChangeLog", "LICENCE"]
  s.homepage = "http://rubyforge.org/projects/ruby-pdf"
  s.rdoc_options = ["--title", "PDF::Writer", "--main", "README", "--line-numbers"]
  s.require_paths = ["lib"]
  s.required_ruby_version = Gem::Requirement.new("> 0.0.0")
  s.rubyforge_project = "ruby-pdf"
  s.rubygems_version = "1.8.19"
  s.summary = "A pure Ruby PDF document creation library."

  if s.respond_to? :specification_version then
    s.specification_version = 1

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<color>, [">= 1.4.0"])
      s.add_runtime_dependency(%q<transaction-simple>, ["~> 1.3"])
    else
      s.add_dependency(%q<color>, [">= 1.4.0"])
      s.add_dependency(%q<transaction-simple>, ["~> 1.3"])
    end
  else
    s.add_dependency(%q<color>, [">= 1.4.0"])
    s.add_dependency(%q<transaction-simple>, ["~> 1.3"])
  end
end
