class ChangeTypeFieldFromRichContent < ActiveRecord::Migration
  def self.up
  	rename_column :rich_contents, :type, :kind
  end

  def self.down
  	rename_column :rich_contents, :kind, :type
  end
end
