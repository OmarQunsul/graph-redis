require 'redis'

redis = Redis.new

describe 'edge increment by' do
  before do
    redis.flushdb
  end

  it "should by able to increment an existing edge" do
    ret = redis.gnode 'graph', 'a', 'b', 'c'
    ret = redis.gedge 'graph', 'a', 'b', 0
    expect(ret).to eq("0")
    ret = redis.gedgeexists 'graph', 'a', 'b'
    expect(ret).to eq(1)
    ret = redis.gedge 'graph', 'a', 'b', 0
    expect(ret).to eq("0")
    ret = redis.gedgeincrby 'graph', 'a', 'b', 2
    expect(ret).to eq("2")
    ret = redis.gedgeincrby 'graph', 'a', 'b', 3.1
    expect(ret).to eq("5.0999999999999996")
    ret = redis.gedgevalue 'graph', 'a', 'b'
    expect(ret).to eq("5.0999999999999996")
  end

  it "should create the edge with that value if it doesnt exist" do
    ret = redis.gnode 'graph', 'a', 'b', 'c'
    ret = redis.gedgeexists 'graph', 'a', 'b'
    expect(ret).to eq(0)
    ret = redis.gedgeincrby 'graph', 'a', 'b', 3
    expect(ret).to eq("3")
    ret = redis.gedgevalue 'graph', 'a', 'b'
    expect(ret).to eq("3")
  end

  it "should return the new edge value as float when created" do
    redis.gnode 'graph', 'a', 'b', 'c'
    ret = redis.gedge 'graph', 'a', 'b', 1.2
    expect(ret).to eq("1.2")
  end
end
