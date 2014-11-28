# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "addressable"
  s.version = "2.2.4"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Bob Aman"]
  s.date = "2011-01-28"
  s.description = "Addressable is a replacement for the URI implementation that is part of\nRuby's standard library. It more closely conforms to the relevant RFCs and\nadds support for IRIs and URI templates.\n"
  s.email = "bob@sporkmonger.com"
  s.extra_rdoc_files = ["README"]
  s.files = ["README"]
  s.homepage = "http://addressable.rubyforge.org/"
  s.rdoc_options = ["--main", "README"]
  s.require_paths = ["lib"]
  s.rubyforge_project = "addressable"
  s.rubygems_version = "1.8.19"
  s.summary = "URI Implementation"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_development_dependency(%q<rake>, [">= 0.7.3"])
      s.add_development_dependency(%q<rspec>, [">= 1.0.8"])
      s.add_development_dependency(%q<launchy>, [">= 0.3.2"])
      s.add_development_dependency(%q<diff-lcs>, [">= 1.1.2"])
    else
      s.add_dependency(%q<rake>, [">= 0.7.3"])
      s.add_dependency(%q<rspec>, [">= 1.0.8"])
      s.add_dependency(%q<launchy>, [">= 0.3.2"])
      s.add_dependency(%q<diff-lcs>, [">= 1.1.2"])
    end
  else
    s.add_dependency(%q<rake>, [">= 0.7.3"])
    s.add_dependency(%q<rspec>, [">= 1.0.8"])
    s.add_dependency(%q<launchy>, [">= 0.3.2"])
    s.add_dependency(%q<diff-lcs>, [">= 1.1.2"])
  end
end
