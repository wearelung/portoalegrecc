# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "rspec"
  s.version = "1.3.2"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["The RSpec Development Team"]
  s.date = "2011-04-11"
  s.description = "Behaviour Driven Development for Ruby."
  s.email = ["rspec-devel@rubyforge.org"]
  s.executables = ["autospec", "spec"]
  s.files = ["bin/autospec", "bin/spec"]
  s.homepage = ""
  s.require_paths = ["lib"]
  s.rubyforge_project = "rspec"
  s.rubygems_version = "1.8.19"
  s.summary = "rspec 1.3.2"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_development_dependency(%q<cucumber>, [">= 0.3"])
      s.add_development_dependency(%q<fakefs>, [">= 0.2.1"])
      s.add_development_dependency(%q<syntax>, [">= 1.0"])
      s.add_development_dependency(%q<diff-lcs>, [">= 1.1.2"])
    else
      s.add_dependency(%q<cucumber>, [">= 0.3"])
      s.add_dependency(%q<fakefs>, [">= 0.2.1"])
      s.add_dependency(%q<syntax>, [">= 1.0"])
      s.add_dependency(%q<diff-lcs>, [">= 1.1.2"])
    end
  else
    s.add_dependency(%q<cucumber>, [">= 0.3"])
    s.add_dependency(%q<fakefs>, [">= 0.2.1"])
    s.add_dependency(%q<syntax>, [">= 1.0"])
    s.add_dependency(%q<diff-lcs>, [">= 1.1.2"])
  end
end
