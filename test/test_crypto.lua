local crypto = require 'crypto'

local html_text = [[
<tt class="descclassname">HMAC.</tt><tt class="descname">hexdigest</tt><big>(</big><big>)</big><a class="headerlink" href="#hmac.HMAC.hexdigest" title="Permalink to this definition">?</a></dt>
<dd><p>Like <a class="reference internal" href="#hmac.HMAC.digest" title="hmac.HMAC.digest"><tt class="xref py py-meth docutils literal"><span class="pre">digest()</span></tt></a> except the digest is returned as a string twice the length
containing only hexadecimal digits.  This may be used to exchange the value
<p class="last">When comparing the output of <a class="reference internal" href="#hmac.HMAC.hexdigest" title="hmac.HMAC.hexdigest"><tt class="xref py py-meth docutils literal"><span class="pre">hexdigest()</span></tt></a> to an externally-supplied
digest during a verification routine, it is recommended to use the
<a class="reference internal" href="#hmac.compare_digest" title="hmac.compare_digest"><tt class="xref py py-func docutils literal"><span class="pre">compare_digest()</span></tt></a> function instead of the <tt class="docutils literal"><span class="pre">==</span></tt> operator
to reduce the vulnerability to timing attacks.</p>]]

local pubkey_text = "\
-----BEGIN RSA PUBLIC KEY-----\
MIGHAoGBALjUhW3l3ZP0MvIL5xzqgO63T6EMmEgCxBE2CCX+PPVk534rJcHmcwZZ\
hnsBOr+qeCnLeX/wM8oQa7mPPSn0y/cMUzfFygwwEXWLp+KuCS6edCs5epG8NvqL\
Ix4kxv9xumMql6j6oLmglEboO/j6djk7MHVlUGvcCYRZiLZTpo3BAgED\
-----END RSA PUBLIC KEY-----\
"
local prikey_text = "\
-----BEGIN RSA PRIVATE KEY-----\
MIICXAIBAAKBgQC41IVt5d2T9DLyC+cc6oDut0+hDJhIAsQRNggl/jz1ZOd+KyXB\
5nMGWYZ7ATq/qngpy3l/8DPKEGu5jz0p9Mv3DFM3xcoMMBF1i6firgkunnQrOXqR\
vDb6iyMeJMb/cbpjKpeo+qC5oJRG6Dv4+nY5OzB1ZVBr3AmEWYi2U6aNwQIBAwKB\
gHs4WPPuk7f4IfaymhNHAJ8k38CzEDAB2At5WsP+005DRP7HboFETK7mWadWJypx\
pXEyUP/1d9wK8nu003FN3U468HbNVhlLmMRS7GAWjKopANEiJsl3J96h+zzoBxIK\
3Yy9MkXK2IYcIz1fkCheXGF7sNSL5eJr2orh4H2l48sDAkEA57ZzvGMioYgcJ2iG\
P+UeeM106VmyFJu8ujAGCVaO1Bf1S6edi2PTdkWOdnvfCW/kfjI4Xi6s7BjMOw3t\
r95/TwJBAMw0EdVlw50kMufczExREOglfJzmsXTfb936PL9l1Y1WGsMkor74HQAZ\
3GJn3WN7woN0/jVI604o58uqDCryXe8CQQCaeaJ9l2xrsBLE8Fl/7hRQiPibkSFj\
En3RdVlbjwniuqOHxROyQoz5g7RO/T9bn+2pdtA+ycidZd18s/PKlFTfAkEAiCK2\
jkPXvhgh7+iIMuC18Bj9ve8g+JT1PqbTKkPjs468ghhsf1ATVWaS7EU+QlKBrPip\
eNtHiXCah8ayx0w+nwJBANC533QDDhUMQvj+yOxxvwq7dgBaOdxuILB3M4cZ4qHs\
frmAihdFxGlhB3jYjcnyNDza4LuOuQHq/JiverlqQfs=\
-----END RSA PRIVATE KEY-----\
"

local function test_crc32c()
    local checksum = crypto.crc32c('123456')
    assert(checksum == 1094021510)
    checksum = crypto.crc32c('c4ca4238a0b923820dcc509a6f75849b')
    assert(checksum == 3715924231)
end

local function test_md5()
    local digest = crypto.md5('1')
    assert(digest == 'c4ca4238a0b923820dcc509a6f75849b') 
    digest = crypto.md5('123456')
    assert(digest == 'e10adc3949ba59abbe56e057f20f883e') 
    digest = crypto.md5('c4ca4238a0b923820dcc509a6f75849b')
    assert(digest == '28c8edde3d61a0411511d3b1866f0636')
end

local function test_sha1()
    local digest = crypto.sha1('1')
    assert(digest == '356a192b7913b04c54574d18c28d46e6395428ab')
    digest = crypto.sha1('123456')
    assert(digest == '7c4a8d09ca3762af61e59520943dc26494f8941b')
    digest = crypto.sha1('c4ca4238a0b923820dcc509a6f75849b')
    assert(digest == '0937afa17f4dc08f3c0e5dc908158370ce64df86')
end

local function test_hmac_md5()
    local key = 'hellokitty'
    local digest = crypto.hmac_md5(key, '1')
    assert(digest == 'f48e9b5d460d699b617d930c976f90a8')
    key = digest
    digest = crypto.hmac_md5(key, '123456')
    assert(digest == '6383499db6f9284d405adce2b72ed2bb')
end

local function test_hmac_sha1()
    local key = 'hellokitty'
    local digest = crypto.hmac_sha1(key, '1')
    assert(digest == 'cedbfba2e745c2e49a9a178638778a9cddae21fc')
    key = digest
    digest = crypto.hmac_sha1(key, '123456')
    assert(digest == '1432325b3516d7f56619787f8bed48effeaa5256')
end

local function test_aes()
    local key = '3516d7f55c2e49a1' -- must be 16 bytes length
    local iv = key
    local aes = crypto.new_aes(key, iv)
    assert(type(aes) == 'userdata')
    
    local encrypted = aes:encrypt(html_text)
    print('length:', #html_text, #encrypted)
    local decrypted = aes:decrypt(encrypted)
    assert(decrypted == html_text)
end

local function test_rsa()
    --local pub, priv = crypto:gen_rsa_keypair()
    local client = crypto.new_rsa()
    local server = crypto.new_rsa()
    client:set_pubkey(pubkey_text)
    server:set_key(prikey_text)
    
    local encrypted = client:pub_encrypt(html_text)
    local decrypted = server:decrypt(encrypted)
    assert(decrypted == html_text)
    
    encrypted = server:encrypt(html_text)
    decrypted = server:pub_decrypt(encrypted)
    assert(decrypted == html_text)
end


test_crc32c()
test_md5()
test_sha1()
test_hmac_md5()
test_hmac_sha1()
test_aes()
test_rsa()

print('crypto passed')