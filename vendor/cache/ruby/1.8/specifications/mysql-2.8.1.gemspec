# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "mysql"
  s.version = "2.8.1"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["TOMITA Masahiro"]
  s.date = "2009-08-21"
  s.description = "This is the MySQL API module for Ruby. It provides the same functions for Ruby\nprograms that the MySQL C API provides for C programs.\n\nThis is a conversion of tmtm's original extension into a proper RubyGems."
  s.email = ["tommy@tmtm.org"]
  s.extensions = ["ext/mysql_api/extconf.rb"]
  s.extra_rdoc_files = ["History.txt", "Manifest.txt", "README.txt"]
  s.files = ["History.txt", "Manifest.txt", "README.txt", "ext/mysql_api/extconf.rb"]
  s.homepage = "http://mysql-win.rubyforge.org"
  s.rdoc_options = ["--main", "README.txt"]
  s.require_paths = ["lib", "ext"]
  s.required_ruby_version = Gem::Requirement.new(">= 1.8.6")
  s.rubyforge_project = "mysql-win"
  s.rubygems_version = "1.8.19"
  s.summary = "This is the MySQL API module for Ruby"

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_development_dependency(%q<rake-compiler>, ["~> 0.5"])
      s.add_development_dependency(%q<hoe>, [">= 2.3.3"])
    else
      s.add_dependency(%q<rake-compiler>, ["~> 0.5"])
      s.add_dependency(%q<hoe>, [">= 2.3.3"])
    end
  else
    s.add_dependency(%q<rake-compiler>, ["~> 0.5"])
    s.add_dependency(%q<hoe>, [">= 2.3.3"])
  end
end
