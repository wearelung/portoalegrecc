require 'test/unit'

# require_relative the old-fashioned way...
# Note that it's important there be no chdir before this require.
file = File.expand_path(File.join(File.dirname(__FILE__), 
                                  '../lib/require_relative'))
require file

class TestRR < Test::Unit::TestCase
  require 'tmpdir'
  def test_basic
    dir = 
      if RUBY_VERSION.start_with?('1.8') && 
          RUBY_COPYRIGHT.end_with?('Yukihiro Matsumoto')
        puts "Note: require_relative doesn't work with Dir.chdir as it does on Rubinius or 1.9"
        '.'
      else
        Dir.tmpdir
      end
    abs_file = RequireRelative.abs_file
    # The chdir is to make things harder for those platforms that 
    # truly support require_relative.
    Dir.chdir(dir) do 
      cur_dir  = File.basename(File.expand_path(File.dirname(abs_file)))
      ['./foo', "../#{cur_dir}/bar"].each_with_index do |suffix, i|
        assert_equal(true, require_relative(suffix), 
                     "check require_relative(#{suffix})")
        # Check that the require_relative worked by checking to see of the
        # class from each file was imported.
        assert_equal(Class, (0 == i ? Foo : Bar).class)
        assert_equal(false, require_relative(suffix), 
                     "require_relative(#{suffix}) second time should be false")
      end
    end
  end
end
