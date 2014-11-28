# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "capistrano-ext"
  s.version = "1.2.1"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["Jamis Buck"]
  s.date = "2008-06-14"
  s.description = "Useful task libraries and methods for Capistrano"
  s.email = "jamis@jamisbuck.org"
  s.extra_rdoc_files = ["CHANGELOG.rdoc", "lib/capistrano/ext/assets/request-counter.rb", "lib/capistrano/ext/monitor.rb", "lib/capistrano/ext/multistage.rb", "lib/capistrano/ext/version.rb", "README"]
  s.files = ["CHANGELOG.rdoc", "lib/capistrano/ext/assets/request-counter.rb", "lib/capistrano/ext/monitor.rb", "lib/capistrano/ext/multistage.rb", "lib/capistrano/ext/version.rb", "README"]
  s.homepage = "http://www.capify.org"
  s.rdoc_options = ["--line-numbers", "--inline-source", "--title", "Capistrano-ext", "--main", "README"]
  s.require_paths = ["lib"]
  s.rubyforge_project = "capistrano-ext"
  s.rubygems_version = "1.8.19"
  s.summary = "Useful task libraries and methods for Capistrano"

  if s.respond_to? :specification_version then
    s.specification_version = 2

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<capistrano>, [">= 1.0.0"])
    else
      s.add_dependency(%q<capistrano>, [">= 1.0.0"])
    end
  else
    s.add_dependency(%q<capistrano>, [">= 1.0.0"])
  end
end
