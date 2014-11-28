# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = "acts-as-taggable-on"
  s.version = "2.0.0.rc1"

  s.required_rubygems_version = Gem::Requirement.new("> 1.3.1") if s.respond_to? :required_rubygems_version=
  s.authors = ["Michael Bleigh"]
  s.date = "2010-03-24"
  s.description = "With ActsAsTaggableOn, you could tag a single model on several contexts, such as skills, interests, and awards. It also provides other advanced functionality."
  s.email = "michael@intridea.com"
  s.extra_rdoc_files = ["README.rdoc"]
  s.files = ["README.rdoc"]
  s.homepage = "http://github.com/mbleigh/acts-as-taggable-on"
  s.rdoc_options = ["--charset=UTF-8"]
  s.require_paths = ["lib"]
  s.rubygems_version = "1.8.19"
  s.summary = "ActsAsTaggableOn is a tagging plugin for Rails that provides multiple tagging contexts on a single model."

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
    else
    end
  else
  end
end
