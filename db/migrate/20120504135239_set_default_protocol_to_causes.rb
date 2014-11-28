class SetDefaultProtocolToCauses < ActiveRecord::Migration
  def self.up
  	change_column_default :causes, :protocol, '-1'
    Cause.update_all ["protocol = ?", '-1']
  end

  def self.down
  	change_column_default :causes, :protocol, nil
  	Cause.update_all ["protocol = ?", nil]
  end
end
